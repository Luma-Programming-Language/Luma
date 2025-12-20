#include "../ast.h"
#include <stdio.h>
#include <string.h>

AstNode *create_module_node(ArenaAllocator *arena, const char *name,
                            const char *doc_comment, int potions,
                            AstNode **body, size_t body_count, size_t line,
                            size_t column) {
  AstNode *node =
      create_preprocessor_node(arena, AST_PREPROCESSOR_MODULE,
                               Node_Category_PREPROCESSOR, line, column);
  node->preprocessor.module.name = (char *)name;
  node->preprocessor.module.doc_comment = (char *)doc_comment;
  node->preprocessor.module.potions = potions;
  node->preprocessor.module.body = body;
  node->preprocessor.module.body_count = body_count;
  node->preprocessor.module.file_path = NULL;
  node->preprocessor.module.tokens = NULL;
  node->preprocessor.module.token_count = 0;
  node->preprocessor.module.scope = NULL;
  return node;
}

AstNode *create_use_node(ArenaAllocator *arena, const char *module_name,
                         const char *alias, size_t line, size_t column) {
  AstNode *node =
      create_preprocessor_node(arena, AST_PREPROCESSOR_USE,
                               Node_Category_PREPROCESSOR, line, column);
  node->preprocessor.use.module_name = module_name;
  node->preprocessor.use.alias = alias;
  return node;
}