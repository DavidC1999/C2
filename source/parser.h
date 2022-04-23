#ifndef _PARSER_H
#define _PARSER_H

#include "tokenizer.h"

#include <stdbool.h>

#define MAX_STATEMENTS_PER_FUNC 1000

enum ParseNodeTypes {
	N_ROOT,
	N_FUNC_DEF,
	N_VAR_DEF,
	N_FUNC_CALL,
	N_VAR_ASSIGN
};

typedef struct ParseNode ParseNode;

typedef struct RootNode {
	int count;
	struct ParseNode* definitions;
} RootNode;

typedef struct FuncDefNode {
	char* name;
	int statement_amt;
	struct ParseNode** statements;
} FuncDefNode;

typedef struct VarDefNode {
	char* name;
} VarDefNode;

typedef struct FuncCallNode {
	char* name;
	bool param_is_var;
	union {
		int param;
		char* var_name;
	};
} FuncCallNode;

typedef struct AssignNode {
	char* name;
	int value;
} AssignNode;

struct ParseNode {
	int type;
	int line;
	union {
		RootNode root_params;
		FuncDefNode func_def_params;
		VarDefNode var_def_params;
		AssignNode assign_params;
		FuncCallNode func_call_params;
	};
};

ParseNode* parse(TokenLL* tokens);
void parser_print_AST(ParseNode* node, int indent);
void free_AST(ParseNode* node);

#endif // _PARSER_H
