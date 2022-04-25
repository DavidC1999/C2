#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin_functions.h"
#include "hashtable/hashtable.h"
#include "parser.h"

#define MAX_USER_FUNCTIONS 100
#define MAX_BUILTIN_FUNCTIONS 100
#define MAX_FUNCTION_NAME_LEN 100
#define MAX_VARIABLE_AMT 100

static int visit_node(ParseNode* node);

typedef struct UserFunc {
    int statement_amt;
    ParseNode** statements;
} UserFunc;

static HashTable* user_functions;

typedef struct BuiltinFunc {
    char name[MAX_FUNCTION_NAME_LEN];
    int (*func)(int);
} BuiltinFunc;

static HashTable* builtin_functions;

static HashTable* variables;

static void panic(char* message, int line) {
    if (line >= 0)
        fprintf(stderr, "Error while interpreting on line %d: %s\n", line, message);
    else
        fprintf(stderr, "Error while interpreting: %s\n", message);
    exit(1);
}

static void init_funcs() {
    builtin_functions = hashtable_new(ANY_T, MAX_BUILTIN_FUNCTIONS);
    hashtable_set(builtin_functions, "print", builtin_print);
    hashtable_set(builtin_functions, "putc", builtin_putc);
    hashtable_set(builtin_functions, "input_num", builtin_input_num);

    user_functions = hashtable_new(ANY_T, MAX_USER_FUNCTIONS);

    // we malloc our own user functions to put in the hashtable
    // make sure they get freed when the hashtable is freed
    hashtable_force_free_values(user_functions);
}

static void define_func(char* name, int line, int statement_amt, ParseNode** statements) {
    UserFunc* new_func = malloc(sizeof(UserFunc));
    new_func->statement_amt = statement_amt;
    new_func->statements = statements;

    if (!hashtable_set(user_functions, name, new_func)) {
        char buffer[100];
        snprintf(buffer, 100, "Unable to define function %s", name);
        panic(buffer, line);
    }
}

static int call_func(char* name, ParseNode* callNode) {
    HashEntry buffer;
    if (hashtable_get(user_functions, &buffer, name)) {
        UserFunc* user_func = buffer.value;
        for (int i = 0; i < user_func->statement_amt; ++i) {
            visit_node(user_func->statements[i]);
        }

        return 0;
    }

    if (hashtable_get(builtin_functions, &buffer, name)) {
        int (*builtin_func)(int) = buffer.value;
        int param = 0;
        if (callNode != NULL) param = visit_node(callNode->func_call_params.param);
        return builtin_func(param);
    }

    char str_buffer[100];
    snprintf(str_buffer, 100, "Unknown function: %s", name);

    if (callNode != NULL)
        panic(str_buffer, callNode->line);
    else
        panic(str_buffer, -1);

    return 0;
}

static int visit_node(ParseNode* node) {
    switch (node->type) {
        case N_FUNC_DEF: {
            char* name = node->func_def_params.name;
            int statement_amt = node->func_def_params.statement_amt;

            ParseNode** statements = node->func_def_params.statements;

            define_func(name, node->line, statement_amt, statements);
            break;
        }
        case N_VAR_DEF: {
            hashtable_set_int(variables, node->var_def_params.name, 0);
            break;
        }
        case N_FUNC_CALL: {
            char* name = node->func_call_params.name;
            return call_func(name, node);
            break;
        }
        case N_VAR_ASSIGN: {
            hashtable_set_int(variables, node->assign_params.name, visit_node(node->assign_params.value));
            break;
        }
        case N_BIN_OP: {
            int left = visit_node(node->bin_op_params.left);
            int right = visit_node(node->bin_op_params.right);

            switch (node->bin_op_params.type) {
                case BINOP_ADD:
                    return left + right;
                case BINOP_SUB:
                    return left - right;
                case BINOP_DIV:
                    return left / right;
                case BINOP_MUL:
                    return left * right;
                case BINOP_EQUAL:
                    return left == right;
                case BINOP_LESS:
                    return left < right;
                case BINOP_LEQUAL:
                    return left <= right;
                case BINOP_GREATER:
                    return left > right;
                case BINOP_GEQUAL:
                    return left >= right;
            }
            break;
        }
        case N_VARIABLE: {
            int buffer;
            if (hashtable_get_int(variables, &buffer, node->var_params.name)) {
                return buffer;
            }

            char strBuffer[100];
            snprintf(strBuffer, 100, "Unknown variable: %s", node->var_params.name);
            panic(strBuffer, node->line);
            break;
        }
        case N_NUMBER: {
            return node->num_params.value;
        }
        case N_IF: {
            if (visit_node(node->cond_params.condition)) {
                visit_node(node->cond_params.statement);
            }
            break;
        }
        case N_WHILE: {
            while (visit_node(node->cond_params.condition)) {
                visit_node(node->cond_params.statement);
            }
            break;
        }
        case N_COMPOUND: {
            size_t statement_amt = node->compound_params.statement_amt;
            for (size_t i = 0; i < statement_amt; ++i) {
                visit_node(node->compound_params.statements[i]);
            }
            break;
        }
        default: {
            panic("Unknown node type", node->line);
        }
    }

    return 0;
}

void interpret(ParseNode* node) {
    variables = hashtable_new(INT_T, MAX_VARIABLE_AMT);

    init_funcs();
    if (node->type != N_ROOT) {
        char buffer[100];
        strncpy(buffer, "Interpreting should start at root node", 100);
        panic(buffer, 0);
    }

    int function_amt = node->root_params.count;
    for (int i = 0; i < function_amt; ++i) {
        ParseNode* definitions = node->root_params.definitions;
        visit_node(&definitions[i]);
    }

    call_func("main", NULL);

#ifdef DEBUG
    char* var_name;
    int var_val;
    printf("Variables:\n");
    while (hashtable_get_next_int(variables, &var_name, &var_val)) {
        printf("%s: %d\n", var_name, var_val);
    }
#endif

    hashtable_free(builtin_functions);
    hashtable_free(user_functions);
    hashtable_free(variables);
}
