#ifndef _PARSER_H
#define _PARSER_H

#include "tokenizer.h"

#define MAX_STATEMENTS_PER_FUNC 1000

enum ParseNodeTypes {
	N_ROOT, // int* count, ParseNode*[] function_definitions
	N_FUNC_DEF, // char* name, int* count, ParseNode* statements
	N_STATEMENT, // ParseNode* function_call
	N_FUNC_CALL // char* name, int* param
};

typedef struct ParseNode ParseNode;

typedef struct RootNode {
	// N_ROOT:
	int count;
	struct ParseNode* function_definitions;
} RootNode;

typedef struct FuncDefNode {
	// N_FUNC_DEF:
	char* name;
	int statement_amt;
	struct ParseNode* statements;
} FuncDefNode;

typedef struct StatementNode {
	// N_STATEMENT:
	struct ParseNode* function_call;
} StatementNode;

typedef struct FuncCallNode {
	// N_FUNCTION_CALL:
	char* name;
	int param;
} FuncCallNode;

struct ParseNode {
	int type;
	int line;
	union {
		RootNode root_params;
		FuncDefNode func_def_params;
		StatementNode statement_params;
		FuncCallNode func_call_params;
	};
};

ParseNode* parse(TokenLL* tokens);
void parser_print_tree(ParseNode* node, int indent);
void free_AST(ParseNode* node);

#endif // _PARSER_H
