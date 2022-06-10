#include "interpreter.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin_functions.h"
#include "hashtable/hashtable.h"
#include "parser.h"
#include "tokenizer.h"

#define MAX_USER_FUNCTIONS 100
#define MAX_BUILTIN_FUNCTIONS 100
#define MAX_FUNCTION_NAME_LEN 100
#define MAX_VARIABLE_AMT 100
#define MAX_STR_AMT 100

#define MAX_CALL_DEPTH 100

static int64_t visit_node(ParseNode* node);

typedef struct UserFunc {
    int64_t param_count;
    char** params;
    ParseNode* statement;
} UserFunc;

static int64_t user_function_ret_val = 0;     // return value for the user function currently running
static bool user_function_returning = false;  // set to true by 'return' statement. Reset by 'call_func'
static HashTable* user_functions;

typedef struct BuiltinFunc {
    char name[MAX_FUNCTION_NAME_LEN];
    int64_t (*func)(builtin_panic_func_t, int64_t, int64_t*);
} BuiltinFunc;

static HashTable* builtin_functions;

static HashTable* global_variables;
static HashTable* var_scopes[MAX_CALL_DEPTH];
static int64_t curr_scope = -1;  // -1 for global scope

static HashTable* global_strings;

static void panic(char* message, int64_t line) {
    if (line >= 0)
        fprintf(stderr, "Error while interpreting on line %ld: %s\n", line, message);
    else
        fprintf(stderr, "Error while interpreting: %s\n", message);
    exit(1);
}

static char* curr_builtin_call = "";
static int64_t curr_builtin_line = 0;
static void builtin_panic(char* message) {
    char buffer[500];
    snprintf(buffer, 500, "Error while running builtin function %s: %s", curr_builtin_call, message);
    panic(buffer, curr_builtin_line);
}

static void var_scope_append() {
    var_scopes[curr_scope] = hashtable_new(ANY_T, MAX_VARIABLE_AMT);
    hashtable_force_free_values(var_scopes[curr_scope]);
    ++curr_scope;
}

static HashTable* var_scope_get_current() {
    return var_scopes[curr_scope - 1];
}

static void var_scope_destroy() {
    hashtable_free(var_scopes[--curr_scope]);
}

static int64_t* var_get_addr(ParseNode* node) {
    if (node == NULL || node->type != N_VARIABLE) {
        char* error = "Trying to get variable value of non-variable node (this is an internal interpreter error)";
        if (node == NULL)
            panic(error, -1);
        else
            panic(error, node->line);
    }

    HashEntry var_entry;
    HashTable* current = var_scope_get_current();
    if (hashtable_get(current, &var_entry, node->variable_info.name)) return (int64_t*)var_entry.value;
    if (hashtable_get(global_variables, &var_entry, node->variable_info.name)) return (int64_t*)var_entry.value;

    char buffer[100];
    snprintf(buffer, 100, "Unknown variable: %s", node->variable_info.name);
    panic(buffer, node->line);

    return 0;
}

static int64_t var_get(ParseNode* node) {
    return *var_get_addr(node);
}

static void var_insert_ptr(char* name, int64_t* ptr, int64_t line) {
    HashEntry _;

    if (curr_scope == -1) {
        if (hashtable_get(global_variables, &_, name)) {
            char buffer[100];
            snprintf(buffer, 100, "Variable with name '%s' already exists", name);
            panic(buffer, line);
        }
        hashtable_set(global_variables, name, ptr);
        return;
    }

    HashTable* current = var_scope_get_current();
    if (hashtable_get(current, &_, name)) {
        char buffer[100];
        snprintf(buffer, 100, "Variable with name '%s' already exists", name);
        panic(buffer, line);
    }

    hashtable_set(current, name, ptr);
}

static void var_define_manual(char* name, int64_t value, int64_t line) {
    int64_t* ptr = malloc(sizeof(int64_t));
    *ptr = value;
    var_insert_ptr(name, ptr, line);
}

static void var_define(ParseNode* node) {
    if (node == NULL || node->type != N_VAR_DEF) {
        char* error = "Trying to define variable with non-variable node (this is an internal interpreter error)";
        if (node == NULL)
            panic(error, -1);
        else
            panic(error, node->line);
    }

    int64_t initial_value = node->var_def_info.initial_val == NULL ? 0 : visit_node(node->var_def_info.initial_val);

    var_define_manual(node->var_def_info.name, initial_value, node->line);
}

static int64_t var_set(ParseNode* node) {
    if (node == NULL || !(node->type == N_BIN_OP && node->bin_operation_info.type == BINOP_ASSIGN)) {
        char* error = "Trying to set variable value of non-variable node (this is an internal interpreter error)";
        if (node == NULL)
            panic(error, -1);
        else
            panic(error, node->line);
    }

    if (node->bin_operation_info.left->type == N_UN_OP && node->bin_operation_info.left->un_operation_info.type == UNOP_DEREF) {  // ptr assign
        int64_t* ptr = (int64_t*)visit_node(node->bin_operation_info.left->un_operation_info.operand);
        int64_t value = visit_node(node->bin_operation_info.right);
        *ptr = value;

        return value;
    } else if (node->bin_operation_info.left->type == N_VARIABLE) {  // var assign
        char* var_name = node->bin_operation_info.left->variable_info.name;
        int64_t value = visit_node(node->bin_operation_info.right);

        HashEntry entry;
        HashTable* current = var_scope_get_current();
        if (hashtable_get(current, &entry, var_name)) {
            int64_t* ptr = (int64_t*)entry.value;
            *ptr = value;
            return value;
        }

        if (hashtable_get(global_variables, &entry, var_name)) {
            int64_t* ptr = (int64_t*)entry.value;
            *ptr = value;
            return value;
        }

        char buffer[100];
        snprintf(buffer, 100, "Trying to assign to unknown variable: %s", var_name);
        panic(buffer, node->line);
    } else {
        char buffer[100];
        snprintf(buffer, 100, "Cannot assign to token of type %s", token_type_to_name[node->bin_operation_info.left->type]);
        panic(buffer, node->line);
    }

    return 0;
}

static int64_t str_get_ptr(ParseNode* str_node) {
    if (str_node->type != N_STRING) {
        panic("Trying to get string pointer from non-string node (this is an interpreter error)", str_node->line);
    }

    char* str = str_node->string_info.contents;

    int64_t output;
    if (hashtable_get_int(global_strings, &output, str)) {
        return output;
    }

    if (hashtable_set_int(global_strings, str, (int64_t)str)) {
        return (int64_t)str;
    }

    panic("Unable to add string to global string space", str_node->line);

    return 0;
}

static void init_funcs() {
    builtin_functions = hashtable_new(ANY_T, MAX_BUILTIN_FUNCTIONS);
    hashtable_set(builtin_functions, "print", builtin_print);
    hashtable_set(builtin_functions, "printu", builtin_printu);
    hashtable_set(builtin_functions, "putc", builtin_putc);
    hashtable_set(builtin_functions, "puts", builtin_puts);
    hashtable_set(builtin_functions, "input_num", builtin_input_num);

    user_functions = hashtable_new(ANY_T, MAX_USER_FUNCTIONS);

    // we malloc our own user functions to put in the hashtable
    // make sure they get freed when the hashtable is freed
    hashtable_force_free_values(user_functions);
}

static void init_strings() {
    global_strings = hashtable_new(INT_T, MAX_STR_AMT);
}

static void free_strings() {
    hashtable_free(global_strings);
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

static int64_t call_func(ParseNode* call_node) {
    HashEntry buffer;
    if (hashtable_get(user_functions, &buffer, call_node->func_call_info.name)) {
        UserFunc* user_func = buffer.value;

        int64_t param_amt_given = call_node->func_call_info.param_count;

        if (param_amt_given != user_func->param_count) {
            char buffer[100];
            snprintf(
                buffer,
                100,
                "Function %s expects %ld arguments, but %ld were given",
                call_node->func_call_info.name,
                user_func->param_count, param_amt_given);
            panic(buffer, call_node->line);
        }

        if (user_func->param_count > MAX_PARAMS_PER_FUNC) {
            panic("Too many parameters passed for function call", call_node->line);
        }

        // determine values to pass before creating scope in order to allow users to use parameters in the current scope
        int64_t params_to_pass[MAX_PARAMS_PER_FUNC];

        for (int64_t i = 0; i < user_func->param_count; ++i) {
            params_to_pass[i] = visit_node(call_node->func_call_info.params[i]);
        }

        var_scope_append();

        for (int64_t i = 0; i < user_func->param_count; ++i) {
            var_define_manual(user_func->params[i], params_to_pass[i], call_node->line);
        }

        user_function_ret_val = 0;
        visit_node(user_func->statement);
        user_function_returning = false;

        var_scope_destroy();

        return user_function_ret_val;
    }

    if (hashtable_get(builtin_functions, &buffer, call_node->func_call_info.name)) {
        int64_t params[call_node->func_call_info.param_count];

        for (int64_t i = 0; i < call_node->func_call_info.param_count; ++i) {
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

#ifdef DEBUG
static void debug(int64_t n) {
    (void)n;
}
#endif

static int64_t visit_node(ParseNode* node) {
    if (user_function_returning) {
        // Function is returning. Stop executing statements in the function.
        return 0;
    }

    switch (node->type) {
        case N_FUNC_DEF: {
            define_func(node);
            break;
        }
        case N_VAR_DEF: {
            var_define(node);
            break;
        }
        case N_ARR_DEF: {
            int64_t size = visit_node(node->arr_def_info.size);
            int64_t* ptr = calloc(size, sizeof(int64_t));

            // Put the pointer into the hastable to automatically free it on deletion of the scope
            // use a '$' because this is not recognized by the tokenizer and can never be a user variable
            char name[MAX_IDENTIFIER_LENGTH];
            snprintf(name, MAX_IDENTIFIER_LENGTH, "$arr_%s", node->arr_def_info.name);
            var_insert_ptr(name, ptr, node->line);

            // define the array name to point to the array
            var_define_manual(node->arr_def_info.name, (int64_t)ptr, node->line);
            break;
        }
        case N_FUNC_CALL: {
            return call_func(node);
            break;
        }
        case N_BIN_OP: {
            int64_t left = visit_node(node->bin_operation_info.left);
            int64_t right = visit_node(node->bin_operation_info.right);

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
                case BINOP_BITAND:
                    return left & right;
                case BINOP_BITOR:
                    return left | right;
                case BINOP_SHLEFT:
                    return left << right;
                case BINOP_SHRIGHT:
                    return left >> right;
                case BINOP_ASSIGN:
                    return var_set(node);
            }
            break;
        }
        case N_UN_OP: {
            switch (node->un_operation_info.type) {
                case UNOP_NEGATE:
                    return -1 * visit_node(node->un_operation_info.operand);
                case UNOP_DEREF:
                    int64_t operand = visit_node(node->un_operation_info.operand);

                    return *(int64_t*)operand;
                case UNOP_GET_ADDR:
                    if (node->un_operation_info.operand->type != N_VARIABLE)
                        panic("Address-of operator expects a variable", node->line);
                    int64_t* addr = var_get_addr(node->un_operation_info.operand);
                    return (int64_t)addr;
            }
            break;
        }
        case N_VARIABLE: {
            return var_get(node);
        }
        case N_NUMBER: {
            return node->number_info.value;
        }
        case N_STRING: {
            return str_get_ptr(node);
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
        case N_RETURN: {
            user_function_ret_val = visit_node(node->return_info.value);
            user_function_returning = true;
            break;
        }
#ifdef DEBUG
        case N_DEBUG: {
            debug(node->debug_info.number);
            break;
        }
#endif
        default: {
            char buffer[100];
            snprintf(buffer, 100, "Unknown node type: %d", node->type);
            panic(buffer, node->line);
        }
    }

    return 0;
}

void interpret(ParseNode* node) {
    global_variables = hashtable_new(ANY_T, MAX_VARIABLE_AMT);

    init_funcs();
    init_strings();
    if (node->type != N_ROOT) {
        char buffer[100];
        strncpy(buffer, "Interpreting should start at root node", 100);
        panic(buffer, 0);
    }

    int64_t function_amt = node->root_info.count;
    for (int64_t i = 0; i < function_amt; ++i) {
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
    HashEntry entry;
    printf("Global variables:\n");
    while (hashtable_get_next(global_variables, &entry)) {
        var_name = entry.key;
        int64_t* ptr = (int64_t*)entry.value;
        int64_t val = *ptr;
        printf("%s (%p): %ld / 0x%lx\n", var_name, ptr, val, (uint64_t)val);
    }
#endif

    hashtable_free(builtin_functions);
    hashtable_free(user_functions);
    hashtable_free(global_variables);
    free_strings();
}
