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
    ParseNode* statement;
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

static void define_func(char* name, int line, ParseNode* statement) {
    UserFunc* new_func = malloc(sizeof(UserFunc));
    new_func->statement = statement;

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
        visit_node(user_func->statement);

        return 0;
    }

    if (hashtable_get(builtin_functions, &buffer, name)) {
        int (*builtin_func)(int) = buffer.value;
        int param = 0;
        if (callNode != NULL) param = visit_node(callNode->func_call_info.param);
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
            char* name = node->func_def_info.name;

            ParseNode* statement = node->func_def_info.statement;

            define_func(name, node->line, statement);
            break;
        }
        case N_VAR_DEF: {
            hashtable_set_int(variables, node->var_def_info.name, 0);
            break;
        }
        case N_FUNC_CALL: {
            char* name = node->func_call_info.name;
            return call_func(name, node);
            break;
        }
        case N_VAR_ASSIGN: {
            int _;
            if (!hashtable_get_int(variables, &_, node->assign_info.name)) {
                char buffer[100];
                snprintf(buffer, 100, "Unknown variable: %s", node->assign_info.name);
                panic(buffer, node->line);
            }
            hashtable_set_int(variables, node->assign_info.name, visit_node(node->assign_info.value));
            break;
        }
        case N_BIN_OP: {
            int left = visit_node(node->bin_operation_info.left);
            int right = visit_node(node->bin_operation_info.right);

            switch (node->bin_operation_info.type) {
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
            if (hashtable_get_int(variables, &buffer, node->variable_info.name)) {
                return buffer;
            }

            char strBuffer[100];
            snprintf(strBuffer, 100, "Unknown variable: %s", node->variable_info.name);
            panic(strBuffer, node->line);
            break;
        }
        case N_NUMBER: {
            return node->number_info.value;
        }
        case N_IF: {
            if (visit_node(node->conditional_info.condition)) {
                visit_node(node->conditional_info.statement);
            }
            break;
        }
        case N_WHILE: {
            while (visit_node(node->conditional_info.condition)) {
                visit_node(node->conditional_info.statement);
            }
            break;
        }
        case N_COMPOUND: {
            size_t statement_amt = node->compound_info.statement_amt;
            for (size_t i = 0; i < statement_amt; ++i) {
                visit_node(node->compound_info.statements[i]);
            }
            break;
        }
        default: {
            char buffer[100];
            snprintf(buffer, 100, "Unknown node type: %d", node->type);
            panic(buffer, node->line);
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

    int function_amt = node->root_info.count;
    for (int i = 0; i < function_amt; ++i) {
        ParseNode** definitions = node->root_info.definitions;
        visit_node(definitions[i]);
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
