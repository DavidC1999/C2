#include "interpreter.h"

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

#define MAX_CALL_DEPTH 100

static int visit_node(ParseNode* node);

typedef struct UserFunc {
    int param_count;
    char** params;
    ParseNode* statement;
} UserFunc;

static HashTable* user_functions;

typedef struct BuiltinFunc {
    char name[MAX_FUNCTION_NAME_LEN];
    int (*func)(builtin_panic_func_t, int, int*);
} BuiltinFunc;

static HashTable* builtin_functions;

static HashTable* global_variables;
static HashTable* var_scopes[MAX_CALL_DEPTH];
static int curr_scope = -1;  // -1 for global scope

static void panic(char* message, int line) {
    if (line >= 0)
        fprintf(stderr, "Error while interpreting on line %d: %s\n", line, message);
    else
        fprintf(stderr, "Error while interpreting: %s\n", message);
    exit(1);
}

static char* curr_builtin_call = "";
static int curr_builtin_line = 0;
static void builtin_panic(char* message) {
    char buffer[500];
    snprintf(buffer, 500, "Error while running builtin function %s: %s", curr_builtin_call, message);
    panic(buffer, curr_builtin_line);
}

static void var_scope_append() {
    var_scopes[curr_scope++] = hashtable_new(INT_T, MAX_VARIABLE_AMT);
}

static HashTable* var_scope_get_current() {
    return var_scopes[curr_scope - 1];
}

static void var_scope_destroy() {
    hashtable_free(var_scopes[--curr_scope]);
}

static int var_get(ParseNode* node) {
    if (node == NULL || node->type != N_VARIABLE) {
        char* error = "Trying to get variable value of non-variable node (this is an internal interpreter error)";
        if (node == NULL)
            panic(error, -1);
        else
            panic(error, node->line);
    }

    int result;
    HashTable* current = var_scope_get_current();
    if (hashtable_get_int(current, &result, node->variable_info.name)) return result;
    if (hashtable_get_int(global_variables, &result, node->variable_info.name)) return result;

    char buffer[100];
    snprintf(buffer, 100, "Unknown variable: %s", node->variable_info.name);
    panic(buffer, node->line);

    return 0;
}

static void var_define_manual(char* name, int value, int line) {
    int _;

    if (curr_scope == -1) {
        if (hashtable_get_int(global_variables, &_, name)) {
            char buffer[100];
            snprintf(buffer, 100, "Variable with name '%s' already exists", name);
            panic(buffer, line);
        }
        hashtable_set_int(global_variables, name, value);
        return;
    }

    HashTable* current = var_scope_get_current();
    if (hashtable_get_int(current, &_, name)) {
        char buffer[100];
        snprintf(buffer, 100, "Variable with name '%s' already exists", name);
        panic(buffer, line);
    }

    hashtable_set_int(current, name, value);
}

static void var_define(ParseNode* node) {
    if (node == NULL || node->type != N_VAR_DEF) {
        char* error = "Trying to define variable with non-variable node (this is an internal interpreter error)";
        if (node == NULL)
            panic(error, -1);
        else
            panic(error, node->line);
    }

    // int _;

    int initial_value = node->var_def_info.initial_val == NULL ? 0 : visit_node(node->var_def_info.initial_val);

    var_define_manual(node->var_def_info.name, initial_value, node->line);
    // if (curr_scope == -1) {
    //     if (hashtable_get_int(global_variables, &_, node->var_def_info.name)) {
    //         char buffer[100];
    //         snprintf(buffer, 100, "Variable with name '%s' already exists", node->var_def_info.name);
    //         panic(buffer, node->line);
    //     }
    //     hashtable_set_int(global_variables, node->var_def_info.name, initial_value);
    //     return;
    // }

    // HashTable* current = var_scope_get_current();
    // if (hashtable_get_int(current, &_, node->var_def_info.name)) {
    //     char buffer[100];
    //     snprintf(buffer, 100, "Variable with name '%s' already exists", node->var_def_info.name);
    //     panic(buffer, node->line);
    // }

    // hashtable_set_int(current, node->var_def_info.name, initial_value);
}

static void var_set(ParseNode* node) {
    if (node == NULL || node->type != N_VAR_ASSIGN) {
        char* error = "Trying to set variable value of non-variable node (this is an internal interpreter error)";
        if (node == NULL)
            panic(error, -1);
        else
            panic(error, node->line);
    }

    int _;
    HashTable* current = var_scope_get_current();
    if (hashtable_get_int(current, &_, node->assign_info.name)) {
        hashtable_set_int(current, node->assign_info.name, visit_node(node->assign_info.value));
        return;
    }

    if (hashtable_get_int(global_variables, &_, node->assign_info.name)) {
        hashtable_set_int(global_variables, node->assign_info.name, visit_node(node->assign_info.value));
        return;
    }

    char buffer[100];
    snprintf(buffer, 100, "Trying to assign to unknown variable: %s", node->assign_info.name);
    panic(buffer, node->line);
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

static void define_func(ParseNode* func_def_node) {
    UserFunc* new_func = malloc(sizeof(UserFunc));
    new_func->param_count = func_def_node->func_def_info.param_count;
    new_func->params = func_def_node->func_def_info.params;
    new_func->statement = func_def_node->func_def_info.statement;

    if (!hashtable_set(user_functions, func_def_node->func_def_info.name, new_func)) {
        char buffer[100];
        snprintf(buffer, 100, "Unable to define function %s", func_def_node->func_def_info.name);
        panic(buffer, func_def_node->line);
    }
}

static int call_func(ParseNode* call_node) {
    HashEntry buffer;
    if (hashtable_get(user_functions, &buffer, call_node->func_call_info.name)) {
        UserFunc* user_func = buffer.value;

        int param_amt_given = call_node->func_call_info.param_count;

        if (param_amt_given != user_func->param_count) {
            char buffer[100];
            snprintf(
                buffer,
                100,
                "Function %s expects %d arguments, but %d were given",
                call_node->func_call_info.name,
                user_func->param_count, param_amt_given);
            panic(buffer, call_node->line);
        }

        var_scope_append();

        for (int i = 0; i < user_func->param_count; ++i) {
            var_define_manual(user_func->params[i], visit_node(call_node->func_call_info.params[i]), call_node->line);
        }

        visit_node(user_func->statement);
        var_scope_destroy();

        return 0;
    }

    if (hashtable_get(builtin_functions, &buffer, call_node->func_call_info.name)) {
        int params[call_node->func_call_info.param_count];

        for (int i = 0; i < call_node->func_call_info.param_count; ++i) {
            params[i] = visit_node(call_node->func_call_info.params[i]);
        }

        builtin_func_t builtin_func = buffer.value;
        curr_builtin_call = call_node->func_call_info.name;
        curr_builtin_line = call_node->line;
        return builtin_func(builtin_panic, call_node->func_call_info.param_count, params);
    }

    char str_buffer[100];
    snprintf(str_buffer, 100, "Unknown function: %s", call_node->func_call_info.name);

    if (call_node != NULL)
        panic(str_buffer, call_node->line);
    else
        panic(str_buffer, -1);

    return 0;
}

static int visit_node(ParseNode* node) {
    switch (node->type) {
        case N_FUNC_DEF: {
            define_func(node);
            break;
        }
        case N_VAR_DEF: {
            var_define(node);
            break;
        }
        case N_FUNC_CALL: {
            return call_func(node);
            break;
        }
        case N_VAR_ASSIGN: {
            var_set(node);
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
            return var_get(node);
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
    global_variables = hashtable_new(INT_T, MAX_VARIABLE_AMT);

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

    curr_scope = 0;

    HashEntry _;
    if (!hashtable_get(user_functions, &_, "main")) {
        panic("Every program must have a main function", -1);
    }

    ParseNode main_call;
    main_call.func_call_info.name = "main";
    main_call.func_call_info.param_count = 0;
    call_func(&main_call);

#ifdef DEBUG
    char* var_name;
    int var_val;
    printf("Global variables:\n");
    while (hashtable_get_next_int(global_variables, &var_name, &var_val)) {
        printf("%s: %d\n", var_name, var_val);
    }
#endif

    hashtable_free(builtin_functions);
    hashtable_free(user_functions);
    hashtable_free(global_variables);
}
