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
    N_IF,
    N_WHILE,
    N_COMPOUND,
};

enum BinOpNodeType {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
    BINOP_EQUAL,
    BINOP_LESS,
    BINOP_LEQUAL,
    BINOP_GREATER,
    BINOP_GEQUAL,
};

extern char* bin_op_node_type_to_string[];

typedef struct ParseNode ParseNode;

typedef struct RootNode {
    int count;
    ParseNode** definitions;
} RootNode;

typedef struct FuncDefNode {
    char* name;
    ParseNode* statement;
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

// For both if statements and while loops
typedef struct ConditionalNode {
    ParseNode* condition;
    ParseNode* statement;
} ConditionalNode;

typedef struct CompoundStatement {
    size_t statement_amt;
    ParseNode** statements;
} CompoundStatement;

struct ParseNode {
    enum ParseNodeTypes type;
    int line;
    union {
        RootNode root_info;
        FuncDefNode func_def_info;
        VarDefNode var_def_info;
        AssignNode assign_info;
        FuncCallNode func_call_info;
        BinOpNode bin_operation_info;
        NumberNode number_info;
        VariableNode variable_info;
        ConditionalNode conditional_info;
        CompoundStatement compound_info;
    };
};

ParseNode* parse(TokenLL* tokens);
void print_AST(ParseNode* node, int indent);
void free_AST(ParseNode* node);

#endif  // _PARSER_H
