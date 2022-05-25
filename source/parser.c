#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    size_t str_length = strlen(tokens->current->name) + 1;  // including '\0'
    char* name = malloc(sizeof(char) * str_length);
    strncpy(name, tokens->current->name, str_length);
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

static ParseNode* get_factor(TokenLL* tokens) {
    if (tokens->current == NULL) {
        panic("unexpected end of token stream", tokens->tail->line);
    }

    // variable assignment
    if (tokens->current->type == T_IDENTIFIER && tokens->current->next->type == T_ASSIGN) {
        size_t str_length = strlen(tokens->current->name) + 1;  // including '\0'
        char* name = malloc(sizeof(char) * str_length);
        strncpy(name, tokens->current->name, str_length);

        advance_token(tokens);
        int64_t line = tokens->current->line;

        advance_token(tokens);

        ParseNode* value = get_expression(tokens);

        ParseNode* result = malloc(sizeof(ParseNode));

        result->type = N_VAR_ASSIGN;
        result->line = line;
        result->assign_info.name = name;
        result->assign_info.value = value;

        return result;
    }

    if (tokens->current->type == T_IDENTIFIER) {
        int64_t line = tokens->current->line;
        if (tokens->current->next != NULL && tokens->current->next->type == T_LPAREN)
            return get_function_call(tokens);

        size_t str_length = strlen(tokens->current->name) + 1;  // including '\0'
        char* name = malloc(sizeof(char) * str_length);
        strncpy(name, tokens->current->name, str_length);

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

        ParseNode* to_deref = get_expression(tokens);

        ParseNode* result = malloc(sizeof(ParseNode));

        if (tokens->current->type == T_ASSIGN) {
            advance_token(tokens);

            result->type = N_PTR_ASSIGN;
            result->line = line;
            result->assign_ptr_info.addr = to_deref;
            result->assign_ptr_info.value = get_expression(tokens);
        } else {
            result->type = N_UN_OP;
            result->line = line;
            result->un_operation_info.type = UNOP_DEREF;
            result->un_operation_info.operand = to_deref;
        }

        return result;
    }

    if (tokens->current->type == T_AMPERSAND) {
        int64_t line = tokens->current->line;
        advance_token(tokens);

        expect_token_type(tokens, T_IDENTIFIER);

        size_t str_length = strlen(tokens->current->name) + 1;  // including '\0'
        char* name = malloc(sizeof(char) * str_length);
        strncpy(name, tokens->current->name, str_length);

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
    snprintf(buffer, 200, "Expected token of type [%s, %s, %s] but found %s instead",
             token_type_to_name[T_IDENTIFIER],
             token_type_to_name[T_NUMBER],
             token_type_to_name[T_LPAREN],
             token_type_to_name[tokens->current->type]);

    int64_t line = tokens->current == NULL ? tokens->tail->line : tokens->current->line;

    panic(buffer, line);

    return NULL;
}

static bool token_type_is_term_binop(TokenLL* tokens) {
    return (tokens->current->type == T_ASTERISK ||
            tokens->current->type == T_SLASH ||
            tokens->current->type == T_EQUAL ||
            tokens->current->type == T_LESS ||
            tokens->current->type == T_LEQUAL ||
            tokens->current->type == T_GREATER ||
            tokens->current->type == T_GEQUAL);
}

static ParseNode* get_term(TokenLL* tokens) {
    ParseNode* result = get_factor(tokens);

    while (tokens->current != NULL && token_type_is_term_binop(tokens)) {
        enum BinOpNodeType type;

        switch (tokens->current->type) {
            case T_ASTERISK:
                type = BINOP_MUL;
                break;
            case T_SLASH:
                type = BINOP_DIV;
                break;
            case T_EQUAL:
                type = BINOP_EQUAL;
                break;
            case T_LESS:
                type = BINOP_LESS;
                break;
            case T_LEQUAL:
                type = BINOP_LEQUAL;
                break;
            case T_GREATER:
                type = BINOP_GREATER;
                break;
            case T_GEQUAL:
                type = BINOP_GEQUAL;
                break;
        }

        int64_t line = tokens->current->line;

        advance_token(tokens);
        ParseNode* rhs = get_factor(tokens);

        ParseNode* temp = malloc(sizeof(ParseNode));
        temp->type = N_BIN_OP;
        temp->line = line;
        temp->bin_operation_info.type = type;
        temp->bin_operation_info.left = result;
        temp->bin_operation_info.right = rhs;

        result = temp;
    }

    return result;
}

static ParseNode* get_expression(TokenLL* tokens) {
    ParseNode* result = get_term(tokens);

    while (tokens->current != NULL && (tokens->current->type == T_PLUS || tokens->current->type == T_MINUS)) {
        enum BinOpNodeType type;
        if (tokens->current->type == T_PLUS) {
            type = BINOP_ADD;
        } else {  // tokens->current->type == T_MINUS
            type = BINOP_SUB;
        }

        int64_t line = tokens->current->line;

        advance_token(tokens);
        ParseNode* rhs = get_term(tokens);

        ParseNode* temp = malloc(sizeof(ParseNode));
        temp->type = N_BIN_OP;
        temp->line = line;
        temp->bin_operation_info.type = type;
        temp->bin_operation_info.left = result;
        temp->bin_operation_info.right = rhs;

        result = temp;
    }

    return result;
}

static ParseNode* get_statement(TokenLL* tokens) {
    if (tokens->current == NULL) {
        panic("unexpected end of token stream", tokens->tail->line);
    }

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

        ParseNode* result = malloc(sizeof(ParseNode));
        result->type = keyword_type == K_IF ? N_IF : N_WHILE;
        result->line = line;
        result->conditional_info.condition = condition;
        result->conditional_info.statement = statement;

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
    size_t identifier_len = strlen(tokens->current->name);
    char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
    strncpy(identifier_name, tokens->current->name, identifier_len);
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

            size_t param_len = strlen(tokens->current->name) + 1;  // including '\0'
            func_params[param_counter] = malloc(sizeof(char) * param_len);
            strncpy(func_params[param_counter], tokens->current->name, param_len);

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
    size_t identifier_len = strlen(tokens->current->name);
    char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
    strncpy(identifier_name, tokens->current->name, identifier_len);
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
        case N_PTR_ASSIGN:
            free_AST(node->assign_ptr_info.addr);
            free_AST(node->assign_ptr_info.value);
            break;
        case N_VAR_ASSIGN:
            free(node->assign_info.name);
            free(node->assign_info.value);
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
        case N_PTR_ASSIGN: {
            print_indent(indent);
            printf("Pointer assignment {\n");

            print_indent(indent + 1);
            printf("Address {\n");
            print_AST(node->assign_ptr_info.addr, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent + 1);
            printf("Value {\n");
            print_AST(node->assign_ptr_info.value, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_VAR_ASSIGN: {
            print_indent(indent);
            printf("Variable assignment {\n");

            print_indent(indent + 1);
            printf("Identifier: %s\n", node->assign_info.name);
            print_indent(indent + 1);
            printf("Value {\n");
            print_AST(node->assign_info.value, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

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
    }
}
