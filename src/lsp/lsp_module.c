#ifdef _WIN32
#include <windows.h>
#endif

#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "lsp.h"

#define LUMA_STD_PATH "/usr/local/lib/luma"

#define NEGATIVE_CACHE_MAX 64
static const char *negative_cache[NEGATIVE_CACHE_MAX];
static size_t      negative_cache_count = 0;

static int negative_cache_contains(const char *module_name) {
  for (size_t i = 0; i < negative_cache_count; i++) {
    if (negative_cache[i] && strcmp(negative_cache[i], module_name) == 0)
      return 1;
  }
  return 0;
}

static void negative_cache_insert(const char *module_name) {
  if (negative_cache_count >= NEGATIVE_CACHE_MAX)
    return;
  char *copy = malloc(strlen(module_name) + 1);
  if (copy) {
    strcpy(copy, module_name);
    negative_cache[negative_cache_count++] = copy;
  }
}

void lsp_negative_cache_clear(void) {
  for (size_t i = 0; i < negative_cache_count; i++) {
    free((void *)negative_cache[i]);
    negative_cache[i] = NULL;
  }
  negative_cache_count = 0;
}


// Extract @module declaration from file content
const char *extract_module_name(const char *content, ArenaAllocator *arena) {
  if (!content)
    return NULL;

  const char *src = content;

  while (*src) {
    while (*src && (isspace(*src) || *src == '/' || *src == '#')) {
      if (*src == '/' && *(src + 1) == '/') {
        while (*src && *src != '\n')
          src++;
      } else if (*src == '/' && *(src + 1) == '*') {
        src += 2;
        while (*src && !(*src == '*' && *(src + 1) == '/'))
          src++;
        if (*src)
          src += 2;
      } else {
        src++;
      }
    }

    if (strncmp(src, "@module", 7) == 0) {
      src += 7;

      while (*src && isspace(*src))
        src++;

      if (*src == '"') {
        src++;
        const char *name_start = src;
        while (*src && *src != '"')
          src++;

        size_t name_len = src - name_start;
        char *module_name = arena_alloc(arena, name_len + 1, 1);
        memcpy(module_name, name_start, name_len);
        module_name[name_len] = '\0';

        return module_name;
      }
    }

    if (*src && !isspace(*src) && *src != '@') {
      break;
    }

    if (*src)
      src++;
  }

  return NULL;
}

void scan_file_for_module(LSPServer *server, const char *file_uri,
                          ArenaAllocator *temp_arena) {
  const char *file_path = lsp_uri_to_path(file_uri, temp_arena);
  if (!file_path)
    return;

  FILE *f = fopen(file_path, "r");
  if (!f)
    return;

  char buffer[1024];
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, f);
  buffer[read] = '\0';
  fclose(f);

  const char *module_name = extract_module_name(buffer, temp_arena);
  if (!module_name)
    return;

  for (size_t i = 0; i < server->module_registry.count; i++) {
    if (strcmp(server->module_registry.entries[i].module_name, module_name) == 0) {
      server->module_registry.entries[i].file_uri =
          arena_strdup(server->arena, file_uri);
      return;
    }
  }

  if (server->module_registry.count >= server->module_registry.capacity) {
    size_t new_capacity = server->module_registry.capacity * 2;
    if (new_capacity == 0)
      new_capacity = 32;

    ModuleRegistryEntry *new_entries =
        arena_alloc(server->arena, new_capacity * sizeof(ModuleRegistryEntry),
                    alignof(ModuleRegistryEntry));

    if (server->module_registry.entries) {
      memcpy(new_entries, server->module_registry.entries,
             server->module_registry.count * sizeof(ModuleRegistryEntry));
    }

    server->module_registry.entries = new_entries;
    server->module_registry.capacity = new_capacity;
  }

  ModuleRegistryEntry *entry =
      &server->module_registry.entries[server->module_registry.count++];
  entry->module_name = arena_strdup(server->arena, module_name);
  entry->file_uri = arena_strdup(server->arena, file_uri);
}

void scan_directory_recursive(LSPServer *server, const char *dir_path,
                              ArenaAllocator *temp_arena) {
#ifdef _WIN32
  WIN32_FIND_DATA find_data;
  char search_path[512];
  snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);

  HANDLE hFind = FindFirstFile(search_path, &find_data);
  if (hFind == INVALID_HANDLE_VALUE)
    return;

  do {
    if (strcmp(find_data.cFileName, ".") == 0 ||
        strcmp(find_data.cFileName, "..") == 0) {
      continue;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path,
             find_data.cFileName);

    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      scan_directory_recursive(server, full_path, temp_arena);
    } else {
      const char *ext = strrchr(find_data.cFileName, '.');
      if (ext && strcmp(ext, ".lx") == 0) {
        const char *uri = lsp_path_to_uri(full_path, temp_arena);
        if (uri) {
          scan_file_for_module(server, uri, temp_arena);
        }
      }
    }
  } while (FindNextFile(hFind, &find_data));

  FindClose(hFind);
#else
  DIR *dir = opendir(dir_path);
  if (!dir)
    return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

    struct stat st;
    if (stat(full_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        scan_directory_recursive(server, full_path, temp_arena);
      } else if (S_ISREG(st.st_mode)) {
        const char *ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".lx") == 0) {
          const char *uri = lsp_path_to_uri(full_path, temp_arena);
          if (uri) {
            scan_file_for_module(server, uri, temp_arena);
          }
        }
      }
    }
  }

  closedir(dir);
#endif
}

void build_module_registry(LSPServer *server, const char *workspace_uri) {
  ArenaAllocator temp_arena;
  arena_allocator_init(&temp_arena, 64 * 1024);

  const char *workspace_path = lsp_uri_to_path(workspace_uri, &temp_arena);
  if (!workspace_path) {
    arena_destroy(&temp_arena);
    return;
  }

  scan_directory_recursive(server, workspace_path, &temp_arena);
  arena_destroy(&temp_arena);
}

const char *lookup_module(LSPServer *server, const char *module_name) {
  if (!module_name) return NULL;

  // 1. Workspace registry (always in-memory, fast)
  for (size_t i = 0; i < server->module_registry.count; i++) {
    if (strcmp(server->module_registry.entries[i].module_name, module_name) == 0)
      return server->module_registry.entries[i].file_uri;
  }

  // 2. Negative cache — skip filesystem probe for known-missing modules
  if (negative_cache_contains(module_name)) {
    fprintf(stderr, "[LSP] Module '%s' in negative cache, skipping probe\n",
            module_name);
    return NULL;
  }

  // 3. One-time check whether the std lib directory exists at all
  static int std_lib_exists = -1;
  if (std_lib_exists == -1) {
    struct stat st;
    std_lib_exists = (stat(LUMA_STD_PATH, &st) == 0 && S_ISDIR(st.st_mode)) ? 1 : 0;
    fprintf(stderr, "[LSP] Std lib path '%s': %s\n", LUMA_STD_PATH,
            std_lib_exists ? "found" : "not found (will skip probes)");
  }

  if (!std_lib_exists) {
    negative_cache_insert(module_name);
    return NULL;
  }

  // 4. Probe std lib filesystem.
  //
  //    Luma stdlib modules use the naming convention:
  //      module name  ->  directory/filename.lx
  //    where the first '_'-delimited segment is the subdirectory and the
  //    remainder is the filename.  Examples:
  //      std_string  ->  std/string.lx
  //      std_vector  ->  std/vector.lx
  //      std_io      ->  std/io.lx
  //      std_memory  ->  std/memory.lx
  //      std_args    ->  std/args.lx
  //
  //    We try the subdir/file pattern first, then fall back to a flat probe
  //    (module_name.lx) for any modules that don't follow the convention.

  // Pattern (a): split on first '_' -> subdir/filename.lx
  const char *underscore = strchr(module_name, '_');
  if (underscore) {
    size_t dir_len       = (size_t)(underscore - module_name);
    const char *filename = underscore + 1;

    size_t path_len = strlen(LUMA_STD_PATH) + dir_len + strlen(filename) + 8;
    char *std_path = malloc(path_len);
    if (std_path) {
      snprintf(std_path, path_len, "%s/%.*s/%s.lx",
               LUMA_STD_PATH, (int)dir_len, module_name, filename);

      FILE *test = fopen(std_path, "r");
      if (test) {
        fclose(test);
        const char *uri = lsp_path_to_uri(std_path, server->arena);
        free(std_path);
        fprintf(stderr, "[LSP] Found module '%s' in std lib: %s\n",
                module_name, uri);
        return uri;
      }
      free(std_path);
    }
  }

  // Pattern (b): flat — LUMA_STD_PATH/module_name.lx
  {
    size_t path_len = strlen(LUMA_STD_PATH) + strlen(module_name) + 10;
    char *std_path = malloc(path_len);
    if (std_path) {
      snprintf(std_path, path_len, "%s/%s.lx", LUMA_STD_PATH, module_name);

      FILE *test = fopen(std_path, "r");
      if (test) {
        fclose(test);
        const char *uri = lsp_path_to_uri(std_path, server->arena);
        free(std_path);
        fprintf(stderr, "[LSP] Found module '%s' in std lib (flat): %s\n",
                module_name, uri);
        return uri;
      }
      free(std_path);
    }
  }

  negative_cache_insert(module_name);
  fprintf(stderr, "[LSP] Module '%s' not found, cached as negative\n",
          module_name);
  return NULL;
}

// Extract @use declarations from source code.
void extract_imports(LSPDocument *doc, ArenaAllocator *arena) {
  if (!doc || !doc->content)
    return;

  GrowableArray imports;
  growable_array_init(&imports, arena, 4, sizeof(ImportedModule));

  const char *src = doc->content;
  int brace_depth = 0;

  while (*src) {
    if (*src == '{') {
      brace_depth++;
      if (brace_depth >= 1) break;
    }
    if (*src == '}') {
      if (brace_depth > 0) brace_depth--;
    }

    if (*src == '/' && *(src + 1) == '/') {
      while (*src && *src != '\n') src++;
      continue;
    }

    if (*src == '/' && *(src + 1) == '*') {
      src += 2;
      while (*src && !(*src == '*' && *(src + 1) == '/')) src++;
      if (*src) src += 2;
      continue;
    }

    if (*src == '"') {
      src++;
      while (*src && *src != '"') {
        if (*src == '\\') src++;
        if (*src) src++;
      }
      if (*src == '"') src++;
      continue;
    }

    if (*src == '@' && strncmp(src, "@use", 4) == 0) {
      const char *after = src + 4;
      if (!isspace((unsigned char)*after) && *after != '\0') {
        src++;
        continue;
      }

      src += 4;
      while (*src && isspace((unsigned char)*src)) src++;

      if (*src == '"') {
        src++;
        const char *path_start = src;
        while (*src && *src != '"') src++;

        size_t path_len = src - path_start;
        char *module_path = arena_alloc(arena, path_len + 1, 1);
        memcpy(module_path, path_start, path_len);
        module_path[path_len] = '\0';

        if (*src == '"') src++;

        while (*src && isspace((unsigned char)*src)) src++;

        const char *alias = NULL;
        if (strncmp(src, "as", 2) == 0 &&
            !isalnum((unsigned char)src[2]) && src[2] != '_') {
          src += 2;
          while (*src && isspace((unsigned char)*src)) src++;

          const char *alias_start = src;
          while (*src && (isalnum((unsigned char)*src) || *src == '_'))
            src++;

          size_t alias_len = src - alias_start;
          char *alias_buf = arena_alloc(arena, alias_len + 1, 1);
          memcpy(alias_buf, alias_start, alias_len);
          alias_buf[alias_len] = '\0';
          alias = alias_buf;
        }

        ImportedModule *import =
            (ImportedModule *)growable_array_push(&imports);
        if (import) {
          import->module_path = module_path;
          import->alias       = alias;
          import->scope       = NULL;
        }
      }
      continue;
    }

    src++;
  }

  doc->imports      = (ImportedModule *)imports.data;
  doc->import_count = imports.count;
}

const char *resolve_module_path(const char *current_uri,
                                const char *module_path,
                                ArenaAllocator *arena) {
  const char *current_path = lsp_uri_to_path(current_uri, arena);
  if (!current_path)
    return NULL;

  const char *last_slash = strrchr(current_path, '/');
  if (!last_slash)
    last_slash = strrchr(current_path, '\\');

  size_t dir_len = last_slash ? (last_slash - current_path + 1) : 0;

  size_t total_len = dir_len + strlen(module_path) + 4;
  char *full_path = arena_alloc(arena, total_len, 1);

  if (dir_len > 0) {
    memcpy(full_path, current_path, dir_len);
  }
  strcpy(full_path + dir_len, module_path);

  if (!strstr(module_path, ".lx")) {
    strcat(full_path, ".lx");
  }

  return lsp_path_to_uri(full_path, arena);
}

void lsp_ast_cache_init(LSPServer *server) {
  memset(server->ast_cache, 0, sizeof(server->ast_cache));
  server->ast_cache_count = 0;
  arena_allocator_init(&server->cache_arena, 4 * 1024 * 1024);
}

void lsp_ast_cache_invalidate(LSPServer *server, const char *uri) {
  for (size_t i = 0; i < server->ast_cache_count; i++) {
    if (server->ast_cache[i].uri &&
        strcmp(server->ast_cache[i].uri, uri) == 0) {
      server->ast_cache[i].ast   = NULL;
      server->ast_cache[i].mtime = 0;
      fprintf(stderr, "[LSP] Cache: invalidated %s\n", uri);
      return;
    }
  }
}

static AstNode *cache_lookup(LSPServer *server, const char *uri, long mtime) {
  for (size_t i = 0; i < server->ast_cache_count; i++) {
    ModuleASTCacheEntry *e = &server->ast_cache[i];
    if (e->uri && strcmp(e->uri, uri) == 0) {
      if (e->ast && e->mtime == mtime) {
        return e->ast;
      }
      return NULL;
    }
  }
  return NULL;
}

static void cache_store(LSPServer *server, const char *uri,
                        AstNode *ast, long mtime) {
  for (size_t i = 0; i < server->ast_cache_count; i++) {
    ModuleASTCacheEntry *e = &server->ast_cache[i];
    if (e->uri && strcmp(e->uri, uri) == 0) {
      e->ast   = ast;
      e->mtime = mtime;
      return;
    }
  }
  if (server->ast_cache_count < MODULE_AST_CACHE_MAX) {
    ModuleASTCacheEntry *e = &server->ast_cache[server->ast_cache_count++];
    e->uri   = arena_strdup(&server->cache_arena, uri);
    e->ast   = ast;
    e->mtime = mtime;
  }
}

AstNode *parse_imported_module_ast(LSPServer *server, const char *module_uri,
                                   BuildConfig *config, ArenaAllocator *arena) {
  // 1. If the file is already open in the editor, use its AST directly.
  LSPDocument *module_doc = lsp_document_find(server, module_uri);
  if (module_doc && module_doc->ast) {
    fprintf(stderr, "[LSP] Cache: open doc hit for %s\n", module_uri);
    return module_doc->ast;
  }

  const char *file_path = lsp_uri_to_path(module_uri, arena);
  if (!file_path)
    return NULL;

  struct stat st;
  long mtime = 0;
  if (stat(file_path, &st) == 0) {
    mtime = (long)st.st_mtime;
  }

  AstNode *cached = cache_lookup(server, module_uri, mtime);
  if (cached) {
    fprintf(stderr, "[LSP] Cache: AST hit for %s\n", module_uri);
    return cached;
  }

  fprintf(stderr, "[LSP] Cache: parsing %s (mtime=%ld)\n", module_uri, (long)mtime);

  FILE *f = fopen(file_path, "r");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *file_content = arena_alloc(&server->cache_arena, size + 1, 1);
  if (!file_content) { fclose(f); return NULL; }
  fread(file_content, 1, size, f);
  file_content[size] = '\0';
  fclose(f);

  Lexer lexer;
  init_lexer(&lexer, file_content, &server->cache_arena);

  GrowableArray tokens;
  growable_array_init(&tokens, &server->cache_arena, 256, sizeof(Token));

  Token token;
  while ((token = next_token(&lexer)).type_ != TOK_EOF) {
    Token *slot = (Token *)growable_array_push(&tokens);
    if (slot) *slot = token;
  }

  AstNode *module_ast = parse(&tokens, &server->cache_arena, config);
  if (!module_ast)
    return NULL;

  AstNode *result = module_ast;
  if (module_ast->type == AST_PROGRAM &&
      module_ast->stmt.program.module_count > 0) {
    result = module_ast->stmt.program.modules[0];
  }

  cache_store(server, module_uri, result, mtime);

  return result;
}

void resolve_imports(LSPServer *server, LSPDocument *doc, BuildConfig *config,
                     GrowableArray *imported_modules) {
  if (!doc || !doc->imports || doc->import_count == 0)
    return;

  for (size_t i = 0; i < doc->import_count; i++) {
    ImportedModule *import = &doc->imports[i];

    const char *resolved_uri = lookup_module(server, import->module_path);

    if (!resolved_uri) {
      continue;
    }

    AstNode *module_ast =
        parse_imported_module_ast(server, resolved_uri, config, doc->arena);

    if (module_ast) {
      AstNode **slot = (AstNode **)growable_array_push(imported_modules);
      if (slot) {
        *slot = module_ast;
      }
    }
  }
}