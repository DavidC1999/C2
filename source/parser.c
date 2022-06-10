#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helperfunctions.h"
#include "tokenizer.h"

char* bin_op_node_type_to_string[] = {
    "BINOP_ADD",
    "BINOP_SUB",
    "BINOP_MUL",
    "BINOP_DIV",
    "BINOP_EQUAL",
    "BINOP_LESS",
    "BINOP_LEQUAL",
    "BINOP_GREATER",
    "BINOP_GEQUAL",
    "BINOP_BITAND",
    "BINOP_BITOR",
    "BINOP_SHLEFT",
    "BINOP_SHRIGHT",
    "BINOP_ASSIGN",
};

char* un_op_node_type_to_string[] = {
    "UN_NEGATE",
    "UNOP_DEREF",
    "UNOP_GET_ADDR",
};

static void advance_token(TokenLL* tokens) {
    if (tokens->current == NULL) return;
    tokens->current = tokens->current->next;
}

static void panic(char* message, int64_t line) {
    fprintf(stderr, "Parser error on line %ld: %s\n", line, message);
    exit(1);
}

static void expect_token_type(TokenLL* tokens, int64_t expected_type) {
    if (tokens->current == NULL) {
        char buffer[100];
        snprintf(buffer, 100, "Expected token type %s, but found end of tokens instead", token_type_to_name[expected_type]);
        panic(buffer, tokens->tail->line);
    }

    if (tokens->current->type != expected_type) {
        char buffer[100];
        snprintf(buffer, 100, "Expected token type %s, but found %s instead", token_type_to_name[expected_type], token_type_to_name[tokens->current->type]);
        panic(buffer, tokens->current->line);
    }
}

static void expect_keyword(TokenLL* tokens, int64_t expected_keyword) {
    expect_token_type(tokens, T_KEYWORD);

    if (tokens->current->number != expected_keyword) {
        char buffer[100];
        snprintf(buffer, 100, "Expected keyword of type %s, but found %s, instead", token_type_to_name[tokens->current->number], token_type_to_name[expected_keyword]);
        panic(buffer, tokens->current->line);
    }
}

static ParseNode* get_expression(TokenLL* tokens);
static ParseNode* get_variable_definition(TokenLL* tokens);

static ParseNode* get_function_call(TokenLL* tokens) {
    expect_token_type(tokens, T_IDENTIFIER);
    size_t str_length = strlen(tokens->current->string) + 1;  // including '\0'
    char* name = malloc(sizeof(char) * str_length);
    strncpy(name, tokens->current->string, str_length);
    int64_t line = tokens->current->line;
    advance_token(tokens);

    expect_token_type(tokens, T_LPAREN);

    // TODO: dynamic array
    ParseNode* func_params[MAX_PARAMS_PER_FUNC];
    int64_t param_counter = 0;

    if (tokens->current->next->type != T_RPAREN) {
        do {
            if (param_counter >= MAX_PARAMS_PER_FUNC) {
                panic("Too many function parameters", tokens->current->line);
            }

            advance_token(tokens);
            func_params[param_counter] = get_expression(tokens);

            ++param_counter;
        } while (tokens->current->type == T_COMMA);
    } else {
        advance_token(tokens);
    }

    expect_token_type(tokens, T_RPAREN);
    advance_token(tokens);

    ParseNode* result = malloc(sizeof(ParseNode));
    result->type = N_FUNC_CALL;
    result->line = line;
    result->func_call_info.name = name;
    result->func_call_info.param_count = param_counter;
    result->func_call_info.params = malloc(sizeof(ParseNode*) * param_counter);
    for (int64_t i = 0; i < param_counter; ++i) {
        result->func_call_info.params[i] = func_params[i];
    }

    return result;
}

static ParseNode* get_arr_addr_calculation(char* arr_name, ParseNode* index, int64_t line) {
    // example: "list[3] = 5" is exactly equivalent to "@(list + 3 * 8) = 5"
    // below, we build up the tree for calculating "list + 3 * 8"
    ParseNode* result = malloc(sizeof(ParseNode));
    result->line = line;
    result->type = N_BIN_OP;
    result->bin_operation_info.type = BINOP_ADD;

    result->bin_operation_info.left = malloc(sizeof(ParseNode));
    result->bin_operation_info.left->line = line;
    result->bin_operation_info.left->type = N_VARIABLE;
    result->bin_operation_info.left->variable_info.name = arr_name;

    result->bin_operation_info.right = malloc(sizeof(ParseNode));
    result->bin_operation_info.right->line = line;
    result->bin_operation_info.right->type = N_BIN_OP;
    result->bin_operation_info.right->bin_operation_info.type = BINOP_MUL;
    result->bin_operation_info.right->bin_operation_info.left = index;

    result->bin_operation_info.right->bin_operation_info.right = malloc(sizeof(ParseNode));
    result->bin_operation_info.right->bin_operation_info.right->line = line;
    result->bin_operation_info.right->bin_operation_info.right->type = N_NUMBER;
    result->bin_operation_info.right->bin_operation_info.right->number_info.value = 8;

    return result;
}

static ParseNode* get_factor(TokenLL* tokens) {
    if (tokens->current == NULL) {
        panic("unexpected end of token stream", tokens->tail->line);
    }

    if (tokens->current->type == T_IDENTIFIER) {
        int64_t line = tokens->current->line;  // store the line number of the identifier for possible later use

        size_t str_length = strlen(tokens->current->string) + 1;  // including '\0'
        char* name = malloc(sizeof(char) * str_length);
        strncpy(name, tokens->current->string, str_length);

        // array access
        if (tokens->current->next->type == T_LSQUARE) {
            advance_token(tokens);  // move to left square
            advance_token(tokens);  // move past left square

            ParseNode* result = malloc(sizeof(ParseNode));

            ParseNode* index = get_expression(tokens);

            expect_token_type(tokens, T_RSQUARE);
            advance_token(tokens);

            result->type = N_UN_OP;
            result->line = line;  // use array identfier line number as line number for node
            result->un_operation_info.type = UNOP_DEREF;
            result->un_operation_info.operand = get_arr_addr_calculation(name, index, line);
            return result;
        }

        free(name);
    }

    if (tokens->current->type == T_IDENTIFIER) {
        int64_t line = tokens->current->line;
        if (tokens->current->next != NULL && tokens->current->next->type == T_LPAREN)
            return get_function_call(tokens);

        size_t str_length = strlen(tokens->current->string) + 1;  // including '\0'
        char* name = malloc(sizeof(char) * str_length);
        strncpy(name, tokens->current->string, str_length);

        advance_token(tokens);

        ParseNode* result = malloc(sizeof(ParseNode*));
        result->type = N_VARIABLE;
        result->line = line;
        result->variable_info.name = name;

        return result;
    }

    if (tokens->current->type == T_NUMBER) {
        ParseNode* result = malloc(sizeof(ParseNode));
        result->type = N_NUMBER;
        result->line = tokens->current->line;
        result->number_info.value = tokens->current->number;
        advance_token(tokens);
        return result;
    }

    if (tokens->current->type == T_STRING) {
        ParseNode* result = malloc(sizeof(ParseNode));
        result->type = N_STRING;
        result->line = tokens->current->line;

        size_t count = strlen(tokens->current->string) + 1;
        result->string_info.contents = malloc(sizeof(char) * count);
        strncpy(result->string_info.contents, tokens->current->string, strlen(tokens->current->string) + 1);

        advance_token(tokens);
        return result;
    }

    if (tokens->current->type == T_LPAREN) {
        advance_token(tokens);
        ParseNode* result = get_expression(tokens);
        expect_token_type(tokens, T_RPAREN);
        advance_token(tokens);
        return result;
    }

    if (tokens->current->type == T_MINUS) {
        int64_t line = tokens->current->line;
        advance_token(tokens);

        ParseNode* operand = get_expression(tokens);

        ParseNode* result = malloc(sizeof(ParseNode));
        result->type = N_UN_OP;
        result->line = line;
        result->un_operation_info.type = UNOP_NEGATE;
        result->un_operation_info.operand = operand;
        return result;
    }

    if (tokens->current->type == T_AT) {
        int64_t line = tokens->current->line;
        advance_token(tokens);

        ParseNode* to_deref;
        if (tokens->current->type == T_LPAREN) {  // Allow dereference with pointer arithmetic within parenthesis
            to_deref = get_expression(tokens);
        } else {  // No parenthesis, expect a variable
            /*
                This is to avoid a scenario like @a = 10
                To be interpreted as @(a = 10)
                Instead of @(a) = 10
             */
            expect_token_type(tokens, T_IDENTIFIER);

            size_t str_length = strlen(tokens->current->string) + 1;  // including '\0'
            char* name = malloc(sizeof(char) * str_length);
            strncpy(name, tokens->current->string, str_length);

            to_deref = malloc(sizeof(ParseNode*));
            to_deref->type = N_VARIABLE;
            to_deref->line = tokens->current->line;
            to_deref->variable_info.name = name;

            advance_token(tokens);
        }

        ParseNode* result = malloc(sizeof(ParseNode));

        result->type = N_UN_OP;
        result->line = line;
        result->un_operation_info.type = UNOP_DEREF;
        result->un_operation_info.operand = to_deref;

        return result;
    }

    if (tokens->current->type == T_AMPERSAND) {
        int64_t line = tokens->current->line;
        advance_token(tokens);

        expect_token_type(tokens, T_IDENTIFIER);

        size_t str_length = strlen(tokens->current->string) + 1;  // including '\0'
        char* name = malloc(sizeof(char) * str_length);
        strncpy(name, tokens->current->string, str_length);

        ParseNode* operand = malloc(sizeof(ParseNode*));
        operand->type = N_VARIABLE;
        operand->line = tokens->current->line;
        operand->variable_info.name = name;

        advance_token(tokens);

        ParseNode* result = malloc(sizeof(ParseNode));
        result->type = N_UN_OP;
        result->line = line;
        result->un_operation_info.type = UNOP_GET_ADDR;
        result->un_operation_info.operand = operand;
        return result;
    }

    char buffer[200];
    snprintf(buffer, 200, "Unexpected token of type %s",
             token_type_to_name[tokens->current->type]);

    int64_t line = tokens->current == NULL ? tokens->tail->line : tokens->current->line;

    panic(buffer, line);

    return NULL;
}

#define HIGHEST_OP_PRECEDENCE 5

static int64_t get_token_type_precedence(TokenLL* tokens) {
    switch (tokens->current->type) {
        case T_ASSIGN:
            return 1;
        case T_EQUAL:
        case T_LESS:
        case T_LEQUAL:
        case T_GREATER:
        case T_GEQUAL:
            return 2;

        case T_AMPERSAND:
        case T_PIPE:
        case T_DBL_GREATER:
        case T_DBL_LESS:
            return 3;

        case T_PLUS:
        case T_MINUS:
            return 4;

        case T_ASTERISK:
        case T_SLASH:
            static_assert(HIGHEST_OP_PRECEDENCE == 5);
            return 5;
    }

    return -1;
}

static int64_t binop_token_to_binop_type(int64_t binop_token) {
    switch (binop_token) {
        // 1:
        case T_ASSIGN:
            return BINOP_ASSIGN;

        // 2:
        case T_EQUAL:
            return BINOP_EQUAL;
        case T_LESS:
            return BINOP_LESS;
        case T_LEQUAL:
            return BINOP_LEQUAL;
        case T_GREATER:
            return BINOP_GREATER;
        case T_GEQUAL:
            return BINOP_GEQUAL;

        // 3:
        case T_AMPERSAND:
            return BINOP_BITAND;
        case T_PIPE:
            return BINOP_BITOR;
        case T_DBL_GREATER:
            return BINOP_SHRIGHT;
        case T_DBL_LESS:
            return BINOP_SHLEFT;

        // 4:
        case T_PLUS:
            return BINOP_ADD;
        case T_MINUS:
            return BINOP_SUB;

        // 5:
        case T_ASTERISK:
            return BINOP_MUL;
        case T_SLASH:
            return BINOP_DIV;
    }

    return -1;
}

static ParseNode* get_expression_recursive(int64_t precedence, TokenLL* tokens) {
    // there are no more binary operators to check, get the factor
    if (precedence > HIGHEST_OP_PRECEDENCE) return get_factor(tokens);

    ParseNode* result = get_expression_recursive(precedence + 1, tokens);

    while (tokens->current != NULL && get_token_type_precedence(tokens) == precedence) {
        enum BinOpNodeType type = binop_token_to_binop_type(tokens->current->type);
        d_assert((int)type != -1);

        int64_t line = tokens->current->line;

        advance_token(tokens);
        ParseNode* rhs = get_expression_recursive(precedence + 1, tokens);

        ParseNode* binop = malloc(sizeof(ParseNode));
        binop->type = N_BIN_OP;
        binop->line = line;
        binop->bin_operation_info.type = type;
        binop->bin_operation_info.left = result;
        binop->bin_operation_info.right = rhs;

        result = binop;
    }

    return result;
}

static ParseNode* get_expression(TokenLL* tokens) {
    return get_expression_recursive(0, tokens);
}

static ParseNode* get_statement(TokenLL* tokens) {
    if (tokens->current == NULL) {
        panic("unexpected end of token stream", tokens->tail->line);
    }

#ifdef DEBUG
    // debug statement
    if (tokens->current->type == T_KEYWORD && tokens->current->number == K_DEBUG) {
        int64_t line = tokens->current->line;
        advance_token(tokens);
        expect_token_type(tokens, T_NUMBER);

        ParseNode* result = malloc(sizeof(ParseNode));
        result->line = line;
        result->type = N_DEBUG;
        result->debug_info.number = tokens->current->number;

        advance_token(tokens);
        expect_token_type(tokens, T_SEMICOLON);
        advance_token(tokens);

        return result;
    }
#endif

    // variable definition
    if (tokens->current->type == T_KEYWORD && tokens->current->number == K_VAR) {
        return get_variable_definition(tokens);
    }

    // if statement & while loop
    if (tokens->current->type == T_KEYWORD && (tokens->current->number == K_IF || tokens->current->number == K_WHILE)) {
        int64_t line = tokens->current->line;
        int64_t keyword_type = tokens->current->number;

        advance_token(tokens);
        expect_token_type(tokens, T_LPAREN);
        advance_token(tokens);

        ParseNode* condition = get_expression(tokens);

        expect_token_type(tokens, T_RPAREN);
        advance_token(tokens);

        ParseNode* statement = get_statement(tokens);
        ParseNode* else_statement;

        if ((keyword_type == K_IF) && (tokens->current->type == T_KEYWORD) && (tokens->current->number = K_ELSE)) {
            advance_token(tokens);
            else_statement = get_statement(tokens);
        }

        ParseNode* result = malloc(sizeof(ParseNode));
        result->type = keyword_type == K_IF ? N_IF : N_WHILE;
        result->line = line;
        result->conditional_info.condition = condition;
        result->conditional_info.statement = statement;
        result->conditional_info.else_statement = else_statement;

        return result;
    }

    // compound statement
    if (tokens->current->type == T_LBRACE) {
        int64_t line = tokens->current->line;

        advance_token(tokens);

        ParseNode* statements[MAX_STATEMENTS_PER_FUNC];

        int64_t statement_counter = 0;
        while (tokens->current != NULL && tokens->current->type != T_RBRACE) {
            if (statement_counter >= MAX_STATEMENTS_PER_FUNC) {
                panic("Too many statements for one compound statement", tokens->current->line);
            }

            statements[statement_counter++] = get_statement(tokens);
        }

        expect_token_type(tokens, T_RBRACE);
        advance_token(tokens);

        ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
        result->type = N_COMPOUND;
        result->line = line;
        result->compound_info.statement_amt = statement_counter;
        result->compound_info.statements = malloc(sizeof(ParseNode*) * statement_counter);

        for (int64_t i = 0; i < statement_counter; ++i) {
            result->compound_info.statements[i] = statements[i];
        }

        return result;
    }

    // return statement
    if (tokens->current->type == T_KEYWORD && tokens->current->number == K_RETURN) {
        int64_t line = tokens->current->line;
        advance_token(tokens);

        ParseNode* value;

        if (tokens->current->type == T_SEMICOLON) {
            value = malloc(sizeof(ParseNode));
            value->type = N_NUMBER;
            value->number_info.value = 0;
        } else {
            value = get_expression(tokens);
        }

        ParseNode* result = malloc(sizeof(ParseNode));
        result->type = N_RETURN;
        result->line = line;
        result->return_info.value = value;

        expect_token_type(tokens, T_SEMICOLON);
        advance_token(tokens);

        return result;
    }

    ParseNode* result = get_expression(tokens);
    expect_token_type(tokens, T_SEMICOLON);
    advance_token(tokens);
    return result;
}

static ParseNode* get_function_definition(TokenLL* tokens) {
    expect_keyword(tokens, K_FUNC);
    int64_t line = tokens->current->line;
    advance_token(tokens);

    expect_token_type(tokens, T_IDENTIFIER);
    size_t identifier_len = strlen(tokens->current->string);
    char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
    strncpy(identifier_name, tokens->current->string, identifier_len);
    identifier_name[identifier_len] = '\0';
    advance_token(tokens);

    expect_token_type(tokens, T_LPAREN);

    // TODO: dynamic array
    char* func_params[MAX_PARAMS_PER_FUNC];
    int64_t param_counter = 0;

    if (tokens->current->next->type == T_IDENTIFIER) {
        do {
            if (param_counter >= MAX_PARAMS_PER_FUNC) {
                panic("Too many function parameters", tokens->current->line);
            }

            advance_token(tokens);
            expect_token_type(tokens, T_IDENTIFIER);

            size_t param_len = strlen(tokens->current->string) + 1;  // including '\0'
            func_params[param_counter] = malloc(sizeof(char) * param_len);
            strncpy(func_params[param_counter], tokens->current->string, param_len);

            advance_token(tokens);

            ++param_counter;
        } while (tokens->current->type == T_COMMA);
    } else {
        advance_token(tokens);
    }

    expect_token_type(tokens, T_RPAREN);
    advance_token(tokens);
    expect_token_type(tokens, T_LBRACE);  // expect a compound statement

    ParseNode* statement = get_statement(tokens);

    ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
    result->type = N_FUNC_DEF;
    result->line = line;
    result->func_def_info.name = identifier_name;
    result->func_def_info.statement = statement;
    result->func_def_info.param_count = param_counter;
    result->func_def_info.params = malloc(sizeof(char*) * param_counter);
    for (int64_t i = 0; i < param_counter; ++i) {
        result->func_def_info.params[i] = func_params[i];
    }

    return result;
}

static ParseNode* get_variable_definition(TokenLL* tokens) {
    expect_keyword(tokens, K_VAR);
    int64_t line = tokens->current->line;
    advance_token(tokens);

    expect_token_type(tokens, T_IDENTIFIER);
    size_t identifier_len = strlen(tokens->current->string);
    char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
    strncpy(identifier_name, tokens->current->string, identifier_len);
    identifier_name[identifier_len] = '\0';
    advance_token(tokens);

    ParseNode* result = malloc(sizeof(ParseNode));
    result->line = line;
    if (tokens->current->type == T_LSQUARE) {
        advance_token(tokens);
        result->type = N_ARR_DEF;
        result->arr_def_info.name = identifier_name;
        result->arr_def_info.size = get_expression(tokens);
        expect_token_type(tokens, T_RSQUARE);
        advance_token(tokens);
    } else {
        result->type = N_VAR_DEF;
        result->var_def_info.name = identifier_name;
        if (tokens->current->type == T_ASSIGN) {
            advance_token(tokens);
            result->var_def_info.initial_val = get_expression(tokens);
        } else {
            result->var_def_info.initial_val = NULL;
        }
    }

    expect_token_type(tokens, T_SEMICOLON);
    advance_token(tokens);

    return result;
}

ParseNode* parse(TokenLL* tokens) {
    // TODO: resizing array
    ParseNode* definitions[1000];
    int64_t definitions_counter = 0;
    while (tokens->current != NULL &&
           tokens->current->type == T_KEYWORD) {
        switch (tokens->current->number) {
            case K_FUNC:
                definitions[definitions_counter++] = get_function_definition(tokens);
                break;
            case K_VAR:
                definitions[definitions_counter++] = get_variable_definition(tokens);
                break;
        }
    }

    if (tokens->current != NULL) {
        char buffer[100];
        snprintf(buffer, 100, "Unexpected token: %s", token_type_to_name[tokens->current->type]);
        panic(buffer, tokens->current->line);
    }

    ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
    result->type = N_ROOT;
    result->line = 0;
    result->root_info.count = definitions_counter;
    result->root_info.definitions = malloc(sizeof(ParseNode*) * definitions_counter);

    for (int64_t i = 0; i < definitions_counter; ++i) {
        result->root_info.definitions[i] = definitions[i];
    }

    return result;
}

void free_AST(ParseNode* node) {
    if (node == NULL) {
        fprintf(stderr, "Error while freeing AST, node is NULL");
        exit(1);
    }

    switch (node->type) {
        case N_ROOT:
            int64_t count = node->root_info.count;
            for (int64_t i = 0; i < count; ++i) {
                free(node->root_info.definitions[i]);
            }
            free(node->root_info.definitions);
            break;
        case N_FUNC_DEF:
            for (size_t i = 0; i < node->func_def_info.param_count; ++i) {
                free(node->func_def_info.params[i]);
            }
            free(node->func_def_info.params);
            free(node->func_def_info.name);
            free(node->func_def_info.statement);
            break;
        case N_VAR_DEF:
            free(node->var_def_info.name);
            free(node->var_def_info.initial_val);
            break;
        case N_ARR_DEF:
            free(node->arr_def_info.name);
            free(node->arr_def_info.size);
            break;
        case N_FUNC_CALL:
            for (int64_t i = 0; i < node->func_call_info.param_count; ++i) {
                free(node->func_call_info.params[i]);
            }
            free(node->func_call_info.params);

            free(node->func_call_info.name);
            break;
        case N_BIN_OP:
            free(node->bin_operation_info.left);
            free(node->bin_operation_info.right);
            break;
        case N_UN_OP:
            free(node->un_operation_info.operand);
            break;
        case N_NUMBER:
            break;
        case N_STRING:
            free(node->string_info.contents);
            break;
        case N_VARIABLE:
            free(node->variable_info.name);
            break;
        case N_IF:
        case N_WHILE:
            free_AST(node->conditional_info.condition);
            free_AST(node->conditional_info.statement);
            break;
        case N_COMPOUND:
            for (size_t i = 0; i < node->compound_info.statement_amt; ++i) {
                free(node->compound_info.statements[i]);
            }

            free(node->compound_info.statements);

            break;
        case N_RETURN:
            free_AST(node->return_info.value);
            break;
#ifdef DEBUG
        case N_DEBUG:
            break;
#endif
    }
    free(node);
}

static void print_indent(int64_t amt) {
    for (int64_t i = 0; i < amt; ++i) {
        printf("  ");
    }
}

void print_AST(ParseNode* node, int64_t indent) {
    if (node == NULL) {
        fprintf(stderr, "Error printing freeing AST, node is NULL");
        exit(1);
    }

    switch (node->type) {
        case N_ROOT: {
            print_indent(indent);
            printf("[\n");

            int64_t def_amt = node->root_info.count;
            ParseNode** definitions = node->root_info.definitions;

            for (int64_t i = 0; i < def_amt; ++i) {
                print_AST(definitions[i], indent + 1);
            }

            print_indent(indent);
            printf("]\n");
            break;
        }
        case N_FUNC_DEF: {
            print_indent(indent);
            printf("Function definition {\n");

            print_indent(indent + 1);
            printf("Name: %s\n", node->func_def_info.name);

            print_indent(indent + 1);
            printf("Params [\n");
            for (size_t i = 0; i < node->func_def_info.param_count; ++i) {
                print_indent(indent + 2);
                printf("%s\n", node->func_def_info.params[i]);
            }
            print_indent(indent + 1);
            printf("]\n");

            print_AST(node->func_def_info.statement, indent + 1);

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_VAR_DEF: {
            print_indent(indent);
            printf("Variable definition {\n");

            print_indent(indent + 1);
            printf("Identifier: %s\n", node->var_def_info.name);

            print_indent(indent + 1);
            if (node->var_def_info.initial_val == NULL) {
                printf("Initial value: none\n");
            } else {
                printf("Initial value {\n");
                print_AST(node->var_def_info.initial_val, indent + 2);
                print_indent(indent + 1);
                printf("}\n");
            }

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_ARR_DEF: {
            print_indent(indent);
            printf("Array definition {\n");

            print_indent(indent + 1);
            printf("Identifier: %s\n", node->arr_def_info.name);

            print_indent(indent + 1);
            printf("Size {\n");
            print_AST(node->arr_def_info.size, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_FUNC_CALL: {
            print_indent(indent);
            printf("Function call {\n");

            print_indent(indent + 1);
            printf("Identifier: %s\n", node->func_call_info.name);
            print_indent(indent + 1);
            printf("Params [\n");
            for (int64_t i = 0; i < node->func_call_info.param_count; ++i)
                print_AST(node->func_call_info.params[i], indent + 2);
            print_indent(indent + 1);
            printf("]\n");

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_BIN_OP: {
            print_indent(indent);
            printf("Binary operation {\n");
            print_indent(indent + 1);
            printf("Type: %s\n", bin_op_node_type_to_string[node->bin_operation_info.type]);

            print_indent(indent + 1);
            printf("Left-hand side {\n");
            print_AST(node->bin_operation_info.left, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent + 1);
            printf("Right-hand side {\n");
            print_AST(node->bin_operation_info.right, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_UN_OP: {
            print_indent(indent);
            printf("Unary operation {\n");
            print_indent(indent + 1);
            printf("Type: %s\n", un_op_node_type_to_string[node->un_operation_info.type]);

            print_indent(indent + 1);
            printf("Operand {\n");
            print_AST(node->un_operation_info.operand, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_NUMBER: {
            print_indent(indent);
            printf("Number: %ld\n", node->number_info.value);
            break;
        }
        case N_STRING: {
            print_indent(indent);
            printf("String: \"%s\"\n", node->string_info.contents);
            break;
        }
        case N_VARIABLE: {
            print_indent(indent);
            printf("Variable: %s\n", node->variable_info.name);
            break;
        }
        case N_WHILE:
        case N_IF: {
            print_indent(indent);
            printf("%s statement {\n", node->type == N_IF ? "If" : "While");
            print_indent(indent + 1);
            printf("Condition {\n");
            print_AST(node->conditional_info.condition, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent + 1);
            printf("Statement {\n");
            print_AST(node->conditional_info.statement, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            if (node->type == N_IF) {
                print_indent(indent + 1);
                printf("Else statement {\n");
                print_AST(node->conditional_info.else_statement, indent + 2);
                print_indent(indent + 1);
                printf("}\n");
            }

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_COMPOUND: {
            print_indent(indent);
            printf("Compound statement {\n");

            print_indent(indent + 1);
            printf("Statements: [\n");
            size_t statement_amt = node->compound_info.statement_amt;
            for (size_t i = 0; i < statement_amt; ++i) {
                print_AST(node->compound_info.statements[i], indent + 2);
            }
            print_indent(indent + 1);
            printf("]\n");

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_RETURN: {
            print_indent(indent);
            printf("Return {\n");
            print_AST(node->return_info.value, indent + 1);
            print_indent(indent);
            printf("}\n");
            break;
        }
#ifdef DEBUG
        case N_DEBUG: {
            print_indent(indent);
            printf("DEBUG: %ld\n", node->debug_info.number);
            break;
        }
#endif
    }
}
