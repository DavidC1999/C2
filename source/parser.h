#ifndef _PARSER_H
#define _PARSER_H

#include <stdbool.h>

#include "tokenizer.h"

#define MAX_STATEMENTS_PER_FUNC 1000

enum ParseNodeTypes {
    N_ROOT,
    N_FUNC_DEF,
    N_VAR_DEF,
    N_FUNC_CALL,
    N_VAR_ASSIGN,
    N_BIN_OP,
    N_NUMBER,
    N_VARIABLE,
};

enum BinOpNodeType {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
};

extern char* bin_op_node_type_to_string[];

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
    ParseNode* param;
} FuncCallNode;

typedef struct AssignNode {
    char* name;
    ParseNode* value;
} AssignNode;

typedef struct BinOpNode {
    enum BinOpNodeType type;
    ParseNode* left;
    ParseNode* right;
} BinOpNode;

typedef struct NumberNode {
    int value;
} NumberNode;

typedef struct VariableNode {
    char* name;
} VariableNode;

struct ParseNode {
    enum ParseNodeTypes type;
    int line;
    union {
        RootNode root_params;
        FuncDefNode func_def_params;
        VarDefNode var_def_params;
        AssignNode assign_params;
        FuncCallNode func_call_params;
        BinOpNode bin_op_params;
        NumberNode num_params;
        VariableNode var_params;
    };
};

ParseNode* parse(TokenLL* tokens);
void print_AST(ParseNode* node, int indent);
void free_AST(ParseNode* node);

#endif  // _PARSER_H
