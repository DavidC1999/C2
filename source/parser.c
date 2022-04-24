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

static void advance_token(TokenLL* tokens) {
    if (tokens->current == NULL) return;
    tokens->current = tokens->current->next;
}

static void panic(char* message, int line) {
    fprintf(stderr, "Parser error on line %d: %s\n", line, message);
    exit(1);
}

static void expect_token_type(TokenLL* tokens, int expected_type) {
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

static void expect_keyword(TokenLL* tokens, int expected_keyword) {
    expect_token_type(tokens, T_KEYWORD);

    if (tokens->current->number != expected_keyword) {
        char buffer[100];
        snprintf(buffer, 100, "Expected keyword of type %s, but found %s, instead", token_type_to_name[tokens->current->number], token_type_to_name[expected_keyword]);
        panic(buffer, tokens->current->line);
    }
}

static ParseNode* get_expression(TokenLL* tokens);

static ParseNode* get_function_call(TokenLL* tokens) {
    expect_token_type(tokens, T_IDENTIFIER);
    size_t str_length = strlen(tokens->current->name) + 1;  // including '\0'
    char* name = malloc(sizeof(char) * str_length);
    strncpy(name, tokens->current->name, str_length);
    int line = tokens->current->line;
    advance_token(tokens);

    expect_token_type(tokens, T_LPAREN);
    advance_token(tokens);

    ParseNode* param;
    if (tokens->current->type != T_RPAREN) {
        param = get_expression(tokens);
    } else {
        // implicitly pass 0:
        param = malloc(sizeof(ParseNode));
        param->type = N_NUMBER;
        param->line = tokens->current->line;
        param->num_params.value = 0;
    }

    expect_token_type(tokens, T_RPAREN);
    advance_token(tokens);

    ParseNode* result = malloc(sizeof(ParseNode));
    result->type = N_FUNC_CALL;
    result->line = line;
    result->func_call_params.name = name;
    result->func_call_params.param = param;

    return result;
}

static ParseNode* get_factor(TokenLL* tokens) {
    if (tokens->current == NULL) {
        panic("unexpected end of token stream", tokens->tail->line);
    }

    switch (tokens->current->type) {
        case T_IDENTIFIER: {
            if (tokens->current->next != NULL && tokens->current->next->type == T_LPAREN)
                return get_function_call(tokens);

            // it's not a function call so it must be a variable
            size_t str_length = strlen(tokens->current->name) + 1;  // including '\0'
            char* name = malloc(sizeof(char) * str_length);
            strncpy(name, tokens->current->name, str_length);

            ParseNode* result = malloc(sizeof(ParseNode*));
            result->type = N_VARIABLE;
            result->line = tokens->current->line;
            result->var_params.name = name;

            advance_token(tokens);
            return result;
        }
        case T_NUMBER: {
            ParseNode* result = malloc(sizeof(ParseNode));
            result->type = N_NUMBER;
            result->line = tokens->current->line;
            result->num_params.value = tokens->current->number;
            advance_token(tokens);
            return result;
        }
        case T_LPAREN: {
            advance_token(tokens);
            ParseNode* result = get_expression(tokens);
            expect_token_type(tokens, T_RPAREN);
            advance_token(tokens);
            return result;
        }
    }

    char buffer[200];
    snprintf(buffer, 200, "Expected token of type [%s, %s, %s] but found %s instead",
             token_type_to_name[T_IDENTIFIER],
             token_type_to_name[T_NUMBER],
             token_type_to_name[T_LPAREN],
             token_type_to_name[tokens->current->type]);

    int line = tokens->current == NULL ? tokens->tail->line : tokens->current->line;

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

        int line = tokens->current->line;

        advance_token(tokens);
        ParseNode* rhs = get_factor(tokens);

        ParseNode* temp = malloc(sizeof(ParseNode));
        temp->type = N_BIN_OP;
        temp->line = line;
        temp->bin_op_params.type = type;
        temp->bin_op_params.left = result;
        temp->bin_op_params.right = rhs;

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

        int line = tokens->current->line;

        advance_token(tokens);
        ParseNode* rhs = get_term(tokens);

        ParseNode* temp = malloc(sizeof(ParseNode));
        temp->type = N_BIN_OP;
        temp->line = line;
        temp->bin_op_params.type = type;
        temp->bin_op_params.left = result;
        temp->bin_op_params.right = rhs;

        result = temp;
    }

    return result;
}

static ParseNode* get_statement(TokenLL* tokens) {
    if (tokens->current == NULL) {
        panic("unexpected end of token stream", tokens->tail->line);
    }

    if (tokens->current->type == T_IDENTIFIER && tokens->current->next->type == T_ASSIGN) {
        size_t str_length = strlen(tokens->current->name) + 1;  // including '\0'
        char* name = malloc(sizeof(char) * str_length);
        strncpy(name, tokens->current->name, str_length);

        advance_token(tokens);
        int line = tokens->current->line;

        advance_token(tokens);

        ParseNode* value = get_expression(tokens);

        ParseNode* result = malloc(sizeof(ParseNode));

        result->type = N_VAR_ASSIGN;
        result->line = line;
        result->assign_params.name = name;
        result->assign_params.value = value;

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
    int line = tokens->current->line;
    advance_token(tokens);

    expect_token_type(tokens, T_IDENTIFIER);
    size_t identifier_len = strlen(tokens->current->name);
    char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
    strncpy(identifier_name, tokens->current->name, identifier_len);
    identifier_name[identifier_len] = '\0';
    advance_token(tokens);

    expect_token_type(tokens, T_LPAREN);
    advance_token(tokens);
    expect_token_type(tokens, T_RPAREN);
    advance_token(tokens);
    expect_token_type(tokens, T_LBRACE);
    advance_token(tokens);

    ParseNode* statements[MAX_STATEMENTS_PER_FUNC];

    int statement_counter = 0;
    while (tokens->current != NULL && tokens->current->type == T_IDENTIFIER) {
        if (statement_counter >= MAX_STATEMENTS_PER_FUNC) {
            panic("Too many statements for one function", tokens->current->line);
        }

        statements[statement_counter++] = get_statement(tokens);
    }

    expect_token_type(tokens, T_RBRACE);
    advance_token(tokens);

    ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
    result->type = N_FUNC_DEF;
    result->line = line;
    result->func_def_params.name = identifier_name;
    result->func_def_params.statement_amt = statement_counter;
    result->func_def_params.statements = malloc(sizeof(ParseNode*) * statement_counter);

    for (int i = 0; i < statement_counter; ++i) {
        result->func_def_params.statements[i] = statements[i];
    }

    return result;
}

static ParseNode* get_variable_definition(TokenLL* tokens) {
    expect_keyword(tokens, K_VAR);
    int line = tokens->current->line;
    advance_token(tokens);

    expect_token_type(tokens, T_IDENTIFIER);
    size_t identifier_len = strlen(tokens->current->name);
    char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
    strncpy(identifier_name, tokens->current->name, identifier_len);
    identifier_name[identifier_len] = '\0';
    advance_token(tokens);

    expect_token_type(tokens, T_SEMICOLON);
    advance_token(tokens);

    ParseNode* result = malloc(sizeof(ParseNode));
    result->type = N_VAR_DEF;
    result->line = line;
    result->var_def_params.name = identifier_name;

    return result;
}

ParseNode* parse(TokenLL* tokens) {
    // TODO: resizing array
    ParseNode* definitions[1000];
    int definitions_counter = 0;
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
    result->root_params.count = definitions_counter;
    result->root_params.definitions = (ParseNode*)malloc(sizeof(ParseNode) * definitions_counter);

    for (int i = 0; i < definitions_counter; ++i) {
        result->root_params.definitions[i].type = definitions[i]->type;
        result->root_params.definitions[i].line = definitions[i]->line;
        result->root_params.definitions[i].func_def_params.name = definitions[i]->func_def_params.name;
        result->root_params.definitions[i].func_def_params.statement_amt = definitions[i]->func_def_params.statement_amt;
        result->root_params.definitions[i].func_def_params.statements = definitions[i]->func_def_params.statements;
        free(definitions[i]);
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
            free(node->root_params.definitions);
            break;
        case N_FUNC_DEF:
            free(node->func_def_params.name);

            for (int i = 0; i < node->func_def_params.statement_amt; ++i) {
                free(node->func_def_params.statements[i]);
            }

            free(node->func_def_params.statements);

            break;
        case N_VAR_DEF:
            free(node->var_def_params.name);
            break;
        case N_FUNC_CALL:
            free(node->func_call_params.param);

            free(node->func_call_params.name);
            break;
        case N_VAR_ASSIGN:
            free(node->assign_params.name);
            free(node->assign_params.value);
            break;
        case N_BIN_OP:
            free(node->bin_op_params.left);
            free(node->bin_op_params.right);
            break;
        case N_NUMBER:
            break;
        case N_VARIABLE:
            free(node->var_params.name);
            break;
    }
    free(node);
}

static void print_indent(int amt) {
    for (int i = 0; i < amt; ++i) {
        printf("  ");
    }
}

void print_AST(ParseNode* node, int indent) {
    if (node == NULL) {
        fprintf(stderr, "Error printing freeing AST, node is NULL");
        exit(1);
    }

    switch (node->type) {
        case N_ROOT: {
            print_indent(indent);
            printf("[\n");

            int def_amt = node->root_params.count;
            ParseNode* definitions = node->root_params.definitions;

            for (int i = 0; i < def_amt; ++i) {
                print_indent(indent + 1);
                if (definitions[i].type == N_FUNC_DEF)
                    printf("Function definition: {\n");
                else if (definitions[i].type == N_VAR_DEF)
                    printf("Variable definition: {\n");
                print_AST(&definitions[i], indent + 2);
                print_indent(indent + 1);
                printf("}\n");
            }

            print_indent(indent);
            printf("]\n");
            break;
        }
        case N_FUNC_DEF: {
            print_indent(indent);
            printf("Name: %s\n", node->func_def_params.name);
            print_indent(indent);
            printf("Statements: [\n");
            int statement_amt = node->func_def_params.statement_amt;
            for (int i = 0; i < statement_amt; ++i) {
                print_AST(node->func_def_params.statements[i], indent + 1);
            }
            print_indent(indent);
            printf("]\n");
            break;
        }
        case N_VAR_DEF: {
            print_indent(indent);
            printf("Identifier: %s\n", node->var_def_params.name);
            break;
        }
        case N_FUNC_CALL: {
            print_indent(indent);
            printf("Function call: {\n");

            print_indent(indent + 1);
            printf("Identifier: %s\n", node->func_call_params.name);
            print_indent(indent + 1);
            printf("Param: {\n");
            if (node->func_call_params.param != NULL)
                print_AST(node->func_call_params.param, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_VAR_ASSIGN: {
            print_indent(indent);
            printf("Variable assignment: {\n");

            print_indent(indent + 1);
            printf("Identifier: %s\n", node->assign_params.name);
            print_indent(indent + 1);
            printf("Value {\n");
            print_AST(node->assign_params.value, indent + 2);
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
            printf("Type: %s\n", bin_op_node_type_to_string[node->bin_op_params.type]);

            print_indent(indent + 1);
            printf("Left-hand side {\n");
            print_AST(node->bin_op_params.left, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent + 1);
            printf("Right-hand side {\n");
            print_AST(node->bin_op_params.right, indent + 2);
            print_indent(indent + 1);
            printf("}\n");

            print_indent(indent);
            printf("}\n");
            break;
        }
        case N_NUMBER: {
            print_indent(indent);
            printf("Number: %d\n", node->num_params.value);
            break;
        }
        case N_VARIABLE: {
            print_indent(indent);
            printf("Variable: %s\n", node->var_params.name);
            break;
        }
    }
}
