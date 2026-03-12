#include "lsp.h"

#include <time.h>

#define DEBOUNCE_MS 500

static struct timespec g_last_change = {0, 0};
static bool g_pending_analysis = false;

static long ms_since(struct timespec *t) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return (now.tv_sec - t->tv_sec) * 1000 + (now.tv_nsec - t->tv_nsec) / 1000000;
}

// Run analysis + publish diagnostics for a document.
// Called either immediately (on save/open) or after debounce (on change).
static void analyze_and_publish(LSPServer *server, LSPDocument *doc,
                                const char *uri, ArenaAllocator *arena) {
  BuildConfig config = {0};
  config.check_mem = true;
  lsp_document_analyze(doc, server, &config);

  size_t diag_count;
  LSPDiagnostic *diagnostics = lsp_diagnostics(doc, &diag_count, arena);

  char *params = (char *)malloc(65536);
  if (params) {
    serialize_diagnostics_to_json(uri, diagnostics, diag_count, params, 65536);
    lsp_send_notification("textDocument/publishDiagnostics", params);
    free(params);
  }

  g_pending_analysis = false;
}

// Check if a pending debounced analysis is due and run it.
// Called at the top of the message loop (from lsp_server_run) so we process
// the debounced typecheck even when no new messages arrive.
void lsp_check_pending_analysis(LSPServer *server) {
  if (!g_pending_analysis)
    return;
  if (ms_since(&g_last_change) < DEBOUNCE_MS)
    return;

  // Find the document that needs re-analysis
  for (size_t i = 0; i < server->document_count; i++) {
    LSPDocument *doc = server->documents[i];
    if (doc && doc->needs_reanalysis) {
      ArenaAllocator temp;
      arena_allocator_init(&temp, 64 * 1024);
      fprintf(stderr, "[LSP] Debounce: running deferred analysis for %s\n",
              doc->uri);
      analyze_and_publish(server, doc, doc->uri, &temp);
      arena_destroy(&temp);
    }
  }
}

void lsp_handle_message(LSPServer *server, const char *message) {
  if (!server || !message)
    return;

  fprintf(stderr, "[LSP] Received message: %.500s...\n", message);

  LSPMethod method = lsp_parse_method(message);
  int request_id = extract_int(message, "id");

  fprintf(stderr, "[LSP] Extracted request_id: %d\n", request_id);

  ArenaAllocator temp_arena;
  arena_allocator_init(&temp_arena, 64 * 1024);

  switch (method) {
  case LSP_METHOD_INITIALIZE: {
    fprintf(stderr, "[LSP] FULL INIT MESSAGE: %s\n", message);
    fprintf(stderr, "[LSP] Handling initialize\n");

    if (request_id >= 0) {
      const char *workspace_uri = extract_string(message, "uri", &temp_arena);
      if (workspace_uri) {
        build_module_registry(server, workspace_uri);
      }

      server->initialized = true;
      const char *capabilities =
          "{"
          "\"capabilities\":{"
          "\"textDocumentSync\":1,"
          "\"hoverProvider\":true,"
          "\"definitionProvider\":true,"
          "\"completionProvider\":{\"triggerCharacters\":[\".\",\":\"]},"
          "\"documentSymbolProvider\":true,"
          /* --- ADD THIS BLOCK --- */
          "\"semanticTokensProvider\":{"
          "\"legend\":{"
          "\"tokenTypes\":["
          "\"namespace\",\"type\",\"typeParameter\","
          "\"function\",\"method\",\"property\","
          "\"variable\",\"parameter\",\"keyword\","
          "\"modifier\",\"comment\",\"string\","
          "\"number\",\"operator\",\"struct\","
          "\"enum\",\"enumMember\""
          "],"
          "\"tokenModifiers\":["
          "\"declaration\",\"definition\","
          "\"readonly\",\"static\",\"defaultLibrary\""
          "]"
          "},"
          "\"full\":true,"
          "\"range\":false"
          "}"
          /* ---------------------- */
          "},"
          "\"serverInfo\":{\"name\":\"Luma LSP\",\"version\":\"0.1.0\"}"
          "}";
      lsp_send_response(request_id, capabilities);
    } else {
      fprintf(stderr, "[LSP] ERROR: No valid request_id for initialize!\n");
    }
    break;
  }

  case LSP_METHOD_INITIALIZED:
    fprintf(stderr, "[LSP] Client initialized\n");
    break;

  case LSP_METHOD_TEXT_DOCUMENT_DID_OPEN: {
    fprintf(stderr, "[LSP] Handling didOpen\n");

    const char *uri = extract_string(message, "uri", &temp_arena);
    const char *text = extract_string(message, "text", &temp_arena);
    int version = extract_int(message, "version");

    if (uri && text) {
      fprintf(stderr, "[LSP] Opening document: %s (version %d)\n", uri,
              version);
      fprintf(stderr, "[LSP] Document content length: %zu\n", strlen(text));

      lsp_ast_cache_invalidate(server, uri);
      lsp_negative_cache_clear();

      LSPDocument *doc = lsp_document_open(server, uri, text, version);
      if (doc) {
        // Always analyze immediately on open
        analyze_and_publish(server, doc, uri, &temp_arena);
      }
    }
    break;
  }

  case LSP_METHOD_TEXT_DOCUMENT_DID_CHANGE: {
    fprintf(stderr, "[LSP] Handling didChange\n");

    const char *uri = extract_string(message, "uri", &temp_arena);
    const char *text = extract_string(message, "text", &temp_arena);
    int version = extract_int(message, "version");

    if (uri && text) {
      lsp_document_update(server, uri, text, version);

      // Mark as pending — debounced analysis will fire after DEBOUNCE_MS
      clock_gettime(CLOCK_MONOTONIC, &g_last_change);
      g_pending_analysis = true;

      fprintf(stderr, "[LSP] didChange: deferred analysis (debounce %dms)\n",
              DEBOUNCE_MS);
    }
    break;
  }

  case LSP_METHOD_TEXT_DOCUMENT_DID_CLOSE: {
    fprintf(stderr, "[LSP] Handling didClose\n");
    const char *uri = extract_string(message, "uri", &temp_arena);
    if (uri) {
      lsp_document_close(server, uri);
    }
    break;
  }

  // textDocument/didSave — treat as immediate re-analysis trigger
  case LSP_METHOD_UNKNOWN: {
    // Check if it's didSave (we don't have a dedicated enum value for it)
    const char *method_val = extract_string(message, "method", &temp_arena);
    if (method_val && strncmp(method_val, "textDocument/didSave", 20) == 0) {
      fprintf(stderr,
              "[LSP] Handling didSave — triggering immediate analysis\n");
      const char *uri = extract_string(message, "uri", &temp_arena);
      if (uri) {
        lsp_ast_cache_invalidate(server, uri);
        lsp_negative_cache_clear();
        LSPDocument *doc = lsp_document_find(server, uri);
        if (doc) {
          doc->needs_reanalysis = true;
          analyze_and_publish(server, doc, uri, &temp_arena);
        }
      }
    } else {
      fprintf(stderr, "[LSP] Unhandled method\n");
    }
    break;
  }

  case LSP_METHOD_TEXT_DOCUMENT_HOVER: {
    fprintf(stderr, "[LSP] Handling hover\n");

    const char *uri = extract_string(message, "uri", &temp_arena);
    LSPPosition position = extract_position(message);

    if (uri) {
      LSPDocument *doc = lsp_document_find(server, uri);
      if (doc) {
        const char *hover_text = lsp_hover(doc, position, &temp_arena);
        if (hover_text) {
          size_t hover_len = strlen(hover_text);
          size_t result_size = hover_len + 128;
          char *result = (char *)malloc(result_size);
          if (result) {
            snprintf(result, result_size,
                     "{\"contents\":{\"kind\":\"markdown\",\"value\":\"%s\"}}",
                     hover_text);
            lsp_send_response(request_id, result);
            free(result);
          } else {
            lsp_send_response(request_id, "null");
          }
        } else {
          lsp_send_response(request_id, "null");
        }
      } else {
        lsp_send_response(request_id, "null");
      }
    }
    break;
  }

  case LSP_METHOD_TEXT_DOCUMENT_DEFINITION: {
    fprintf(stderr, "[LSP] Handling definition\n");

    const char *uri = extract_string(message, "uri", &temp_arena);
    LSPPosition position = extract_position(message);

    if (uri) {
      LSPDocument *doc = lsp_document_find(server, uri);
      if (doc) {
        LSPLocation *loc = lsp_definition(doc, position, &temp_arena);
        if (loc) {
          char result[1024];
          snprintf(result, sizeof(result),
                   "{\"uri\":\"%s\",\"range\":{\"start\":{\"line\":%d,"
                   "\"character\":%d},"
                   "\"end\":{\"line\":%d,\"character\":%d}}}",
                   loc->uri, loc->range.start.line, loc->range.start.character,
                   loc->range.end.line, loc->range.end.character);
          lsp_send_response(request_id, result);
        } else {
          lsp_send_response(request_id, "null");
        }
      } else {
        lsp_send_response(request_id, "null");
      }
    }
    break;
  }

  case LSP_METHOD_TEXT_DOCUMENT_COMPLETION: {
    fprintf(stderr, "[LSP] Handling completion\n");

    const char *uri = extract_string(message, "uri", &temp_arena);
    LSPPosition position = extract_position(message);

    fprintf(stderr, "[LSP] Completion: uri=%s line=%d character=%d\n",
            uri ? uri : "NULL", position.line, position.character);

    if (!uri) {
      lsp_send_response(request_id, "{\"items\":[]}");
      break;
    }

    LSPDocument *doc = lsp_document_find(server, uri);
    if (!doc) {
      fprintf(stderr, "[LSP] Completion: document not found\n");
      lsp_send_response(request_id, "{\"items\":[]}");
      break;
    }

    size_t count = 0;
    LSPCompletionItem *items =
        lsp_completion(doc, position, &count, &temp_arena);

    fprintf(stderr, "[LSP] Completion: got %zu items\n", count);

    if (!items || count == 0) {
      lsp_send_response(request_id, "{\"items\":[]}");
      break;
    }

    size_t result_size = count * 512 + 64;
    char *result = (char *)malloc(result_size);
    if (!result) {
      lsp_send_response(request_id, "{\"items\":[]}");
      break;
    }

    serialize_completion_items(items, count, result, result_size);

    fprintf(
        stderr,
        "[LSP] Completion: sending response (id=%d, items=%zu, bytes=%zu)\n",
        request_id, count, strlen(result));

    lsp_send_response(request_id, result);
    free(result);
    break;
  }

  case LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_SYMBOL: {
    fprintf(stderr, "[LSP] Handling documentSymbol\n");

    const char *uri = extract_string(message, "uri", &temp_arena);

    if (!uri) {
      lsp_send_response(request_id, "[]");
      break;
    }

    LSPDocument *doc = lsp_document_find(server, uri);
    if (!doc) {
      fprintf(stderr, "[LSP] documentSymbol: document not found for %s\n", uri);
      lsp_send_response(request_id, "[]");
      break;
    }

    ArenaAllocator sym_arena;
    arena_allocator_init(&sym_arena, 64 * 1024);

    size_t sym_count = 0;
    LSPDocumentSymbol **symbols =
        lsp_document_symbols(doc, &sym_count, &sym_arena);

    if (!symbols || sym_count == 0) {
      arena_destroy(&sym_arena);
      lsp_send_response(request_id, "[]");
      break;
    }

    size_t buf_size = sym_count * 512 + 16;
    char *result = (char *)malloc(buf_size);
    if (!result) {
      arena_destroy(&sym_arena);
      lsp_send_response(request_id, "[]");
      break;
    }

    size_t off = 0;
    off += snprintf(result + off, buf_size - off, "[");
    for (size_t i = 0; i < sym_count && off < buf_size - 2; i++) {
      LSPDocumentSymbol *sym = symbols[i];
      if (i > 0)
        off += snprintf(result + off, buf_size - off, ",");
      off += snprintf(
          result + off, buf_size - off,
          "{\"name\":\"%s\",\"kind\":%d,"
          "\"range\":{\"start\":{\"line\":%d,\"character\":%d},"
          "\"end\":{\"line\":%d,\"character\":%d}},"
          "\"selectionRange\":{\"start\":{\"line\":%d,\"character\":%d},"
          "\"end\":{\"line\":%d,\"character\":%d}}}",
          sym->name ? sym->name : "", (int)sym->kind, sym->range.start.line,
          sym->range.start.character, sym->range.end.line,
          sym->range.end.character, sym->selection_range.start.line,
          sym->selection_range.start.character, sym->selection_range.end.line,
          sym->selection_range.end.character);
    }
    snprintf(result + off, buf_size - off, "]");

    fprintf(stderr, "[LSP] documentSymbol: sending %zu symbols\n", sym_count);
    lsp_send_response(request_id, result);
    free(result);
    arena_destroy(&sym_arena);
    break;
  }

  case LSP_METHOD_TEXT_DOCUMENT_SEMANTIC_TOKENS: {
    fprintf(stderr, "[LSP] Handling semanticTokens/full\n");
    const char *uri = extract_string(message, "uri", &temp_arena);
    if (!uri) {
      lsp_send_response(request_id, "{\"data\":[]}");
      break;
    }

    LSPDocument *doc = lsp_document_find(server, uri);
    if (!doc) {
      lsp_send_response(request_id, "{\"data\":[]}");
      break;
    }

    char *result = lsp_semantic_tokens_full(doc, &temp_arena);
    lsp_send_response(request_id, result ? result : "{\"data\":[]}");
    break;
  }

  case LSP_METHOD_SHUTDOWN:
    fprintf(stderr, "[LSP] Handling shutdown\n");
    lsp_send_response(request_id, "null");
    break;

  case LSP_METHOD_EXIT:
    fprintf(stderr, "[LSP] Exiting\n");
    exit(0);
    break;

  default:
    fprintf(stderr, "[LSP] Unhandled method\n");
    break;
  }

  arena_destroy(&temp_arena);
}