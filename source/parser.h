#ifndef _PARSER_H
#define _PARSER_H

#include <stdbool.h>
#include <stdint.h>

#include "tokenizer.h"

#define MAX_STATEMENTS_PER_FUNC 1000
#define MAX_PARAMS_PER_FUNC 100

enum ParseNodeTypes {
    N_ROOT,
    N_FUNC_DEF,
    N_VAR_DEF,
    N_ARR_DEF,
    N_FUNC_CALL,
    N_PTR_ASSIGN,
    N_VAR_ASSIGN,
    N_BIN_OP,
    N_UN_OP,
    N_NUMBER,
    N_VARIABLE,
    N_IF,
    N_WHILE,
    N_COMPOUND,
    N_RETURN,
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

enum UnOpNodeType {
    UNOP_NEGATE,
    UNOP_DEREF,
    UNOP_GET_ADDR,
};

extern char* bin_op_node_type_to_string[];
extern char* un_op_node_type_to_string[];

typedef struct ParseNode ParseNode;

typedef struct RootNode {
    int64_t count;
    ParseNode** definitions;
} RootNode;

typedef struct FuncDefNode {
    char* name;
    ParseNode* statement;
    size_t param_count;
    char** params;
} FuncDefNode;

typedef struct VarDefNode {
    char* name;
    ParseNode* initial_val;
} VarDefNode;

typedef struct ArrDefNode {
    char* name;
    ParseNode* size;
} ArrDefNode;

typedef struct FuncCallNode {
    char* name;
    int64_t param_count;
    ParseNode** params;
} FuncCallNode;

typedef struct AssignPtrNode {
    ParseNode* addr;
    ParseNode* value;
} AssignPtrNode;

typedef struct AssignNode {
    char* name;
    ParseNode* value;
} AssignNode;

typedef struct BinOpNode {
    enum BinOpNodeType type;
    ParseNode* left;
    ParseNode* right;
} BinOpNode;

typedef struct UnOpNode {
    enum UnOpNodeType type;
    ParseNode* operand;
} UnOpNode;

typedef struct NumberNode {
    int64_t value;
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

typedef struct ReturnStatement {
    ParseNode* value;
} ReturnStatement;

struct ParseNode {
    enum ParseNodeTypes type;
    int64_t line;
    union {
        RootNode root_info;
        FuncDefNode func_def_info;
        VarDefNode var_def_info;
        ArrDefNode arr_def_info;
        AssignPtrNode assign_ptr_info;
        AssignNode assign_info;
        FuncCallNode func_call_info;
        BinOpNode bin_operation_info;
        UnOpNode un_operation_info;
        NumberNode number_info;
        VariableNode variable_info;
        ConditionalNode conditional_info;
        CompoundStatement compound_info;
        ReturnStatement return_info;
    };
};

ParseNode* parse(TokenLL* tokens);
void print_AST(ParseNode* node, int64_t indent);
void free_AST(ParseNode* node);

#endif  // _PARSER_H
