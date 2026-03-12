#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsp.h"

// Skip over a JSON string value starting AT the opening '"'.
// Returns pointer to the character AFTER the closing '"', or NULL on error.
static const char *skip_json_string(const char *p) {
  if (!p || *p != '"')
    return NULL;
  p++; // skip opening quote
  while (*p) {
    if (*p == '\\') {
      p++; // skip escape char
      if (!*p)
        return NULL;
      if (*p == 'u') {
        // \uXXXX — skip 4 hex digits
        for (int i = 0; i < 4 && *(p + 1); i++)
          p++;
      }
    } else if (*p == '"') {
      return p + 1; // past closing quote
    }
    p++;
  }
  return NULL; // unterminated string
}

// Find the value pointer for `key` in `json`, skipping over string contents
// so we never accidentally match a key name that appears inside a string value.
// Returns a pointer to the first non-whitespace character of the value,
// or NULL if not found.
// Exported as find_json_value for use in lsp_message.c.
const char *find_json_value(const char *json, const char *key) {
  if (!json || !key)
    return NULL;

  size_t key_len = strlen(key);
  const char *p = json;

  while (*p) {
    // Skip string values so we don't match keys inside them
    if (*p == '"') {
      // Read the full quoted token to get its exact content
      const char *str_start = p + 1;
      const char *after = skip_json_string(p);
      if (!after)
        break;

      size_t span = (size_t)(after - p - 2); // length between the quotes

      // Only consider this an exact key match — no substring matches
      if (span == key_len && strncmp(str_start, key, key_len) == 0) {
        // Confirm it's a key: skip whitespace and look for ':'
        const char *q = after;
        while (*q && isspace(*q))
          q++;
        if (*q == ':') {
          q++; // skip ':'
          while (*q && isspace(*q))
            q++;
          return q;
        }
      }
      // Not our key — jump past the whole string
      p = after;
      continue;
    }
    p++;
  }

  return NULL;
}

char *extract_string(const char *json, const char *key, ArenaAllocator *arena) {
  const char *value = find_json_value(json, key);
  if (!value) {
    fprintf(stderr, "[LSP] extract_string: key '%s' not found\n", key);
    return NULL;
  }

  if (*value != '"') {
    fprintf(stderr, "[LSP] extract_string: expected string value for key '%s'\n",
            key);
    return NULL;
  }
  value++; // skip opening quote

  // Allocate a worst-case buffer (unescaping never grows the string)
  size_t max_len = strlen(value);
  char *result = arena_alloc(arena, max_len + 1, 1);
  if (!result) {
    fprintf(stderr, "[LSP] extract_string: allocation failed\n");
    return NULL;
  }

  char *dst = result;
  const char *src = value;

  while (*src && *src != '"') {
    if (*src == '\\') {
      src++;
      if (!*src)
        break;
      switch (*src) {
      case 'n':  *dst++ = '\n'; break;
      case 't':  *dst++ = '\t'; break;
      case 'r':  *dst++ = '\r'; break;
      case '"':  *dst++ = '"';  break;
      case '\\': *dst++ = '\\'; break;
      case '/':  *dst++ = '/';  break;
      case 'b':  *dst++ = '\b'; break;
      case 'f':  *dst++ = '\f'; break;
      case 'u': {
        // \uXXXX — simplified: emit '?' and skip the 4 hex digits
        *dst++ = '?';
        for (int i = 0; i < 4 && *(src + 1); i++)
          src++;
        break;
      }
      default:
        *dst++ = *src;
        break;
      }
      src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';

  fprintf(stderr, "[LSP] extract_string: key '%s' = '%.200s'\n", key, result);
  return result;
}

int extract_int(const char *json, const char *key) {
  const char *value = find_json_value(json, key);
  if (!value) {
    fprintf(stderr, "[LSP] extract_int: key '%s' not found\n", key);
    return -1;
  }

  if (*value == '-' || isdigit((unsigned char)*value)) {
    int v = atoi(value);
    fprintf(stderr, "[LSP] extract_int: key '%s' = %d\n", key, v);
    return v;
  }

  // Handle JSON null explicitly
  if (strncmp(value, "null", 4) == 0) {
    fprintf(stderr, "[LSP] extract_int: key '%s' is null\n", key);
    return -1;
  }

  fprintf(stderr, "[LSP] extract_int: unexpected value for key '%s'\n", key);
  return -1;
}

LSPPosition extract_position(const char *json) {
  LSPPosition pos = {0, 0};

  // Seek to the "position" object first so we don't pick up other
  // "line"/"character" keys that might appear earlier in the message.
  const char *position_obj = find_json_value(json, "position");
  if (!position_obj)
    return pos;

  pos.line      = extract_int(position_obj, "line");
  pos.character = extract_int(position_obj, "character");
  return pos;
}

LSPMethod lsp_parse_method(const char *json) {
  if (!json) {
    fprintf(stderr, "[LSP] parse_method: NULL input\n");
    return LSP_METHOD_UNKNOWN;
  }

  fprintf(stderr, "[LSP] parse_method: checking message: %.200s\n", json);

  const char *method_value = find_json_value(json, "method");
  if (!method_value) {
    fprintf(stderr, "[LSP] parse_method: no 'method' field found\n");
    return LSP_METHOD_UNKNOWN;
  }

  if (*method_value == '"') {
    method_value++;
  }

  fprintf(stderr, "[LSP] parse_method: found method starting with: %.50s\n",
          method_value);

  // Longest match first to avoid partial matches
  if (strncmp(method_value, "textDocument/documentSymbol", 27) == 0)
    return LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_SYMBOL;
  if (strncmp(method_value, "textDocument/semanticTokens", 27) == 0)
    return LSP_METHOD_TEXT_DOCUMENT_SEMANTIC_TOKENS;
  if (strncmp(method_value, "textDocument/completion", 23) == 0)
    return LSP_METHOD_TEXT_DOCUMENT_COMPLETION;
  if (strncmp(method_value, "textDocument/definition", 23) == 0)
    return LSP_METHOD_TEXT_DOCUMENT_DEFINITION;
  if (strncmp(method_value, "textDocument/didChange", 22) == 0)
    return LSP_METHOD_TEXT_DOCUMENT_DID_CHANGE;
  if (strncmp(method_value, "textDocument/didClose", 21) == 0)
    return LSP_METHOD_TEXT_DOCUMENT_DID_CLOSE;
  if (strncmp(method_value, "textDocument/didOpen", 20) == 0)
    return LSP_METHOD_TEXT_DOCUMENT_DID_OPEN;
  if (strncmp(method_value, "textDocument/hover", 18) == 0)
    return LSP_METHOD_TEXT_DOCUMENT_HOVER;

  // "initialized" must be checked before "initialize"
  if (strncmp(method_value, "initialized", 11) == 0)
    return LSP_METHOD_INITIALIZED;
  if (strncmp(method_value, "initialize", 10) == 0)
    return LSP_METHOD_INITIALIZE;

  if (strncmp(method_value, "shutdown", 8) == 0)
    return LSP_METHOD_SHUTDOWN;
  if (strncmp(method_value, "exit", 4) == 0)
    return LSP_METHOD_EXIT;

  fprintf(stderr, "[LSP] parse_method: unknown method\n");
  return LSP_METHOD_UNKNOWN;
}

void lsp_send_response(int id, const char *result) {
  // Use heap buffer so large responses (completions) don't overflow
  size_t result_len = result ? strlen(result) : 4;
  size_t buf_size = result_len + 128;
  char *json_msg = (char *)malloc(buf_size);
  if (!json_msg)
    return;

  int msg_len = snprintf(json_msg, buf_size,
                         "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}",
                         id, result ? result : "null");

  printf("Content-Length: %d\r\n\r\n%s", msg_len, json_msg);
  fflush(stdout);
  free(json_msg);
}

void lsp_send_notification(const char *method, const char *params) {
  size_t params_len = params ? strlen(params) : 2;
  size_t buf_size = strlen(method) + params_len + 64;
  char *json_msg = (char *)malloc(buf_size);
  if (!json_msg)
    return;

  int msg_len = snprintf(json_msg, buf_size,
                         "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s}",
                         method, params ? params : "{}");

  printf("Content-Length: %d\r\n\r\n%s", msg_len, json_msg);
  fflush(stdout);
  free(json_msg);
}

void lsp_send_error(int id, int code, const char *message) {
  char json_msg[512];
  int msg_len = snprintf(json_msg, sizeof(json_msg),
                         "{\"jsonrpc\":\"2.0\",\"id\":%d,\"error\":{"
                         "\"code\":%d,\"message\":\"%s\"}}",
                         id, code, message ? message : "");

  printf("Content-Length: %d\r\n\r\n%s", msg_len, json_msg);
  fflush(stdout);
}

// Write a JSON-escaped version of `src` into `dst`, stopping before
// dst_end - 1 to leave room for the NUL terminator.
// Returns number of bytes written (excluding NUL).
static size_t json_escape(char *dst, size_t dst_size, const char *src) {
  char *out = dst;
  char *end = dst + dst_size - 1; // reserve 1 for '\0'
  while (*src && out < end) {
    switch (*src) {
    case '"':
      if (out + 1 >= end) goto done;
      *out++ = '\\'; *out++ = '"';  break;
    case '\\':
      if (out + 1 >= end) goto done;
      *out++ = '\\'; *out++ = '\\'; break;
    case '\n':
      if (out + 1 >= end) goto done;
      *out++ = '\\'; *out++ = 'n';  break;
    case '\r':
      if (out + 1 >= end) goto done;
      *out++ = '\\'; *out++ = 'r';  break;
    case '\t':
      if (out + 1 >= end) goto done;
      *out++ = '\\'; *out++ = 't';  break;
    default:
      *out++ = *src; break;
    }
    src++;
  }
done:
  *out = '\0';
  return (size_t)(out - dst);
}

void serialize_diagnostics_to_json(const char *uri, LSPDiagnostic *diagnostics,
                                   size_t diag_count, char *output,
                                   size_t output_size) {
  size_t offset = 0;
  offset += snprintf(output + offset, output_size - offset,
                     "{\"uri\":\"%s\",\"diagnostics\":[", uri);

  for (size_t i = 0; i < diag_count && offset < output_size - 2; i++) {
    LSPDiagnostic *diag = &diagnostics[i];
    if (i > 0)
      offset += snprintf(output + offset, output_size - offset, ",");

    char escaped_msg[4096];
    json_escape(escaped_msg, sizeof(escaped_msg),
                diag->message ? diag->message : "");

    offset += snprintf(
        output + offset, output_size - offset,
        "{\"range\":{\"start\":{\"line\":%d,\"character\":%d},"
        "\"end\":{\"line\":%d,\"character\":%d}},"
        "\"severity\":%d,\"message\":\"%s\",\"source\":\"%s\"}",
        diag->range.start.line, diag->range.start.character,
        diag->range.end.line,   diag->range.end.character,
        diag->severity, escaped_msg,
        diag->source ? diag->source : "luma");
  }

  snprintf(output + offset, output_size - offset, "]}");
}

void serialize_completion_items(LSPCompletionItem *items, size_t count,
                                char *output, size_t output_size) {
  size_t offset = 0;
  offset += snprintf(output + offset, output_size - offset, "{\"items\":[");

  for (size_t i = 0; i < count && offset < output_size - 2; i++) {
    LSPCompletionItem *item = &items[i];
    if (i > 0)
      offset += snprintf(output + offset, output_size - offset, ",");

    offset += snprintf(output + offset, output_size - offset,
                       "{\"label\":\"%s\",\"kind\":%d",
                       item->label ? item->label : "", item->kind);

    if (item->insert_text) {
      char escaped[2048];
      json_escape(escaped, sizeof(escaped), item->insert_text);
      offset += snprintf(output + offset, output_size - offset,
                         ",\"insertText\":\"%s\",\"insertTextFormat\":%d",
                         escaped, item->format);
    }

    if (item->detail) {
      char escaped_detail[512];
      json_escape(escaped_detail, sizeof(escaped_detail), item->detail);
      offset += snprintf(output + offset, output_size - offset,
                         ",\"detail\":\"%s\"", escaped_detail);
    }

    if (item->sort_text) {
      offset += snprintf(output + offset, output_size - offset,
                         ",\"sortText\":\"%s\"", item->sort_text);
    }

    offset += snprintf(output + offset, output_size - offset, "}");
  }

  snprintf(output + offset, output_size - offset, "]}");
}