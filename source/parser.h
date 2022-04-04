#ifndef _PARSER_H
#define _PARSER_H

#include "tokenizer.h"

#define MAX_STATEMENTS_PER_FUNC 1000

enum ParseNodeTypes {
	N_ROOT, // ParseNode* function_definition
	N_FUNC_DEF, // char* name, int* count, ParseNode* statements
	N_STATEMENT, // ParseNode* function_call
	N_FUNC_CALL // char* name, int* param
};

typedef struct ParseNode {
	int type;
	int line;
	void* data;
} ParseNode;

ParseNode* parse(TokenLL* tokens);
void parser_print_tree(ParseNode* node, int indent);
void free_AST(ParseNode* node);

#endif // _PARSER_H
