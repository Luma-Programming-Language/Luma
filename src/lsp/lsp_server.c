/**
 * @file lsp_server.c
 * @brief Language Server Protocol implementation with type error diagnostics
 */

#include "lsp.h"
#include <dirent.h>
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

static void crash_handler(int sig) {
  const char *name = "UNKNOWN";
  switch (sig) {
  case SIGSEGV: name = "SIGSEGV (segfault)";     break;
  case SIGABRT: name = "SIGABRT (abort/assert)";  break;
  case SIGBUS:  name = "SIGBUS (bus error)";       break;
  case SIGFPE:  name = "SIGFPE (float exception)"; break;
  case SIGILL:  name = "SIGILL (illegal instr)";   break;
  }
  fprintf(stderr, "\n[LSP] *** CRASH: signal %d (%s) ***\n", sig, name);
  void *frames[64];
  int n = backtrace(frames, 64);
  backtrace_symbols_fd(frames, n, fileno(stderr));
  fflush(stderr);
  signal(sig, SIG_DFL);
  raise(sig);
}

static void install_crash_handlers(void) {
  signal(SIGSEGV, crash_handler);
  signal(SIGABRT, crash_handler);
  signal(SIGBUS,  crash_handler);
  signal(SIGFPE,  crash_handler);
  signal(SIGILL,  crash_handler);
}

#define WATCHDOG_SECS 10

typedef struct {
  pthread_t        thread;
  volatile int     active;
  volatile int     done;
  pthread_mutex_t  mu;
  pthread_cond_t   cv;
} Watchdog;

static Watchdog g_watchdog;

static void *watchdog_thread(void *arg) {
  (void)arg;
  pthread_mutex_lock(&g_watchdog.mu);
  while (!g_watchdog.done) {
    if (g_watchdog.active) {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += WATCHDOG_SECS;

      int rc = pthread_cond_timedwait(&g_watchdog.cv, &g_watchdog.mu, &ts);
      if (rc == ETIMEDOUT && g_watchdog.active && !g_watchdog.done) {
        fprintf(stderr,
                "[LSP] *** WATCHDOG: message handler hung for >%d seconds ***\n"
                "[LSP] Killing process so VS Code can restart the server.\n",
                WATCHDOG_SECS);
        fflush(stderr);
        _exit(1);
      }
    } else {
      pthread_cond_wait(&g_watchdog.cv, &g_watchdog.mu);
    }
  }
  pthread_mutex_unlock(&g_watchdog.mu);
  return NULL;
}

static void watchdog_arm(void) {
  pthread_mutex_lock(&g_watchdog.mu);
  g_watchdog.active = 1;
  pthread_cond_signal(&g_watchdog.cv);
  pthread_mutex_unlock(&g_watchdog.mu);
}

static void watchdog_disarm(void) {
  pthread_mutex_lock(&g_watchdog.mu);
  g_watchdog.active = 0;
  pthread_cond_signal(&g_watchdog.cv);
  pthread_mutex_unlock(&g_watchdog.mu);
}

static void watchdog_init(void) {
  pthread_mutex_init(&g_watchdog.mu, NULL);
  pthread_cond_init(&g_watchdog.cv, NULL);
  g_watchdog.active = 0;
  g_watchdog.done   = 0;
  pthread_create(&g_watchdog.thread, NULL, watchdog_thread, NULL);
}

const char *lsp_uri_to_path(const char *uri, ArenaAllocator *arena) {
  if (!uri) return NULL;
  if (strncmp(uri, "file://", 7) == 0) {
    const char *path = uri + 7;
#ifdef _WIN32
    if (path[0] == '/' && path[2] == ':') path++;
#endif
    return arena_strdup(arena, path);
  }
  return arena_strdup(arena, uri);
}

const char *lsp_path_to_uri(const char *path, ArenaAllocator *arena) {
  if (!path) return NULL;
  size_t len = strlen(path) + 10;
  char *uri = arena_alloc(arena, len, 1);
  if (!uri) return NULL;
#ifdef _WIN32
  snprintf(uri, len, "file:///%s", path);
#else
  snprintf(uri, len, "file://%s", path);
#endif
  return uri;
}

bool lsp_server_init(LSPServer *server, ArenaAllocator *arena) {
  if (!server || !arena) return false;

  install_crash_handlers();
  watchdog_init();
  lsp_ast_cache_init(server);

  server->arena = arena;
  server->initialized = false;
  server->client_process_id = -1;
  server->document_count = 0;
  server->document_capacity = 64;

  server->documents =
      arena_alloc(arena, server->document_capacity * sizeof(LSPDocument *),
                  alignof(LSPDocument *));

  server->module_registry.entries = NULL;
  server->module_registry.count   = 0;
  server->module_registry.capacity = 0;

  return server->documents != NULL;
}

void lsp_server_run(LSPServer *server) {
  if (!server) return;

  fprintf(stderr, "[LSP] Server started, waiting for messages...\n");
  fflush(stderr);

  int stdin_fd = fileno(stdin);
  char header_buf[256];

  while (1) {
    // Wait up to 100ms for stdin to be readable, then check debounce
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(stdin_fd, &fds);
    struct timeval tv = {0, 100 * 1000}; // 100ms

    int ready = select(stdin_fd + 1, &fds, NULL, NULL, &tv);

    if (ready < 0) {
      if (errno == EINTR) continue;
      fprintf(stderr, "[LSP] select error: %s\n", strerror(errno));
      break;
    }

    if (ready == 0) {
      // Timeout — check if debounced analysis is due
      lsp_check_pending_analysis(server);
      continue;
    }

    // stdin has data — read next message
    if (!fgets(header_buf, sizeof(header_buf), stdin)) {
      fprintf(stderr, "[LSP] stdin closed, exiting\n");
      break;
    }

    if (strncmp(header_buf, "Content-Length:", 15) != 0)
      continue;

    int content_length = atoi(header_buf + 15);

    if (content_length <= 0) {
      fprintf(stderr, "[LSP] Invalid content_length: %d, skipping\n",
              content_length);
      continue;
    }
    if (content_length > 10 * 1024 * 1024) {
      fprintf(stderr, "[LSP] content_length %d exceeds 10MB cap, skipping\n",
              content_length);
      continue;
    }

    fprintf(stderr, "[LSP] Content-Length: %d\n", content_length);

    while (fgets(header_buf, sizeof(header_buf), stdin)) {
      if (strcmp(header_buf, "\r\n") == 0 || strcmp(header_buf, "\n") == 0)
        break;
    }

    char *message = (char *)malloc(content_length + 1);
    if (!message) {
      fprintf(stderr, "[LSP] Failed to allocate %d bytes, exiting\n",
              content_length);
      break;
    }

    size_t total_read = 0;
    while (total_read < (size_t)content_length) {
      size_t n = fread(message + total_read, 1,
                       (size_t)content_length - total_read, stdin);
      if (n == 0) {
        if (feof(stdin) || ferror(stdin)) {
          fprintf(stderr, "[LSP] stdin EOF/error after %zu/%d bytes\n",
                  total_read, content_length);
          free(message);
          goto done;
        }
        break;
      }
      total_read += n;
    }

    if (total_read != (size_t)content_length) {
      fprintf(stderr, "[LSP] Truncated: expected %d, got %zu — skipping\n",
              content_length, total_read);
      free(message);
      continue;
    }

    message[total_read] = '\0';

    fprintf(stderr, "[LSP] Dispatching (%zu bytes)\n", total_read);
    fflush(stderr);

    watchdog_arm();
    lsp_handle_message(server, message);
    watchdog_disarm();

    free(message);

    fprintf(stderr, "[LSP] Message handled OK\n");
    fflush(stderr);
  }

done:
  fprintf(stderr, "[LSP] Server loop exited\n");
  fflush(stderr);
}

void lsp_server_shutdown(LSPServer *server) {
  if (!server) return;

  for (size_t i = 0; i < server->document_count; i++) {
    if (server->documents[i] && server->documents[i]->arena)
      arena_destroy(server->documents[i]->arena);
  }

  server->initialized = false;
  server->document_count = 0;
}