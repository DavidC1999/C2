#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "tokenizer.h"

void parser_advance_token(TokenLL* tokens) {
	if(tokens->current->next == NULL) return;
	tokens->current = tokens->current->next;
}

static void panic(char* message, int line) {
	fprintf(stderr, "Parser error on line %d: %s\n", line, message);
	exit(1);
}

void parser_expect_token_type(Token* token, int expected_type) {
	if(token->type != expected_type) {
		char buffer[100];
		snprintf(buffer, 100, "Expected token type %s, but found %s instead", token_type_to_name[expected_type], token_type_to_name[token->type]);
		panic(buffer, token->line);
	}
}

void parser_expect_keyword(Token* token, int expected_keyword) {
	parser_expect_token_type(token, T_KEYWORD);

	if(*(int*)(token->data) != expected_keyword) {
		char buffer[100];
		snprintf(buffer, 100, "Expected keyword of type %s, but found %s, instead", token_type_to_name[*(int*)token->data], token_type_to_name[expected_keyword]);
		panic(buffer, token->line);
	}
}

ParseNode* parser_get_function_call(TokenLL* tokens) {
	parser_expect_token_type(tokens->current, T_IDENTIFIER);
	int line = tokens->current->line;

	size_t name_len = strlen(tokens->current->data);
	char* name = (char*)malloc(sizeof(char) * (name_len + 1));
	strncpy(name, tokens->current->data, name_len);
	name[name_len] = '\0';
	parser_advance_token(tokens);

	parser_expect_token_type(tokens->current, T_LPAREN);
	parser_advance_token(tokens);

	parser_expect_token_type(tokens->current, T_NUMBER);
	int* param = (int*)malloc(sizeof(int));
	*param = *(int*)tokens->current->data;

	parser_advance_token(tokens);

	parser_expect_token_type(tokens->current, T_RPAREN);
	parser_advance_token(tokens);

	ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
	result->type = N_FUNC_CALL;
	result->line = line;
	result->data = malloc(sizeof(void*) * 2);
	((char**)(result->data))[0] = name;
	((int**)(result->data))[1] = param;

	return result;
}

ParseNode* parser_get_statement(TokenLL* tokens) {
	ParseNode* func_call = parser_get_function_call(tokens);
	parser_expect_token_type(tokens->current, T_SEMICOLON);
	parser_advance_token(tokens);

	ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode*));
	result->type = N_STATEMENT;
	result->line = func_call->line;
	((ParseNode**)(result->data))[0] = func_call;

	return result;
}

ParseNode* parser_get_function_definition(TokenLL* tokens) {
	parser_expect_keyword(tokens->current, K_FUNC);
	int line = tokens->current->line;
	parser_advance_token(tokens);
	
	parser_expect_token_type(tokens->current, T_IDENTIFIER);
	size_t identifier_len = strlen(tokens->current->data);
	char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
	strncpy(identifier_name, tokens->current->data, identifier_len);
	identifier_name[identifier_len] = '\0';
	parser_advance_token(tokens);

	parser_expect_token_type(tokens->current, T_LPAREN);
	parser_advance_token(tokens);
	parser_expect_token_type(tokens->current, T_RPAREN);
	parser_advance_token(tokens);
	parser_expect_token_type(tokens->current, T_LBRACE);
	parser_advance_token(tokens);

	ParseNode* statement = parser_get_statement(tokens);

	parser_expect_token_type(tokens->current, T_RBRACE);
	parser_advance_token(tokens);

	ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
	result->type = N_FUNC_DEF;
	result->line = line;
	result->data = malloc(sizeof(void*) * 2);
	((char**)(result->data))[0] = identifier_name;
	((ParseNode**)(result->data))[1] = statement;

	return result;
}

ParseNode* parse(TokenLL* tokens) {
	ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
	result->type = N_ROOT;

	if(tokens->head == NULL) {
		result->data = NULL;
	} else {
		result->data = parser_get_function_definition(tokens);
	}
	result->line = 0;
	return result;
}

void free_AST(ParseNode* node) {
	if(node->data != NULL) { 
		switch(node->type) {
			case N_ROOT:
				free_AST(node->data);
				break;
			case N_FUNC_DEF:
				free(((char**)(node->data))[0]);
				free_AST(((ParseNode**)(node->data))[1]);
				break;
			case N_STATEMENT:
				free_AST(((ParseNode**)(node->data))[0]);
				break;
			case N_FUNC_CALL:
				free(((char**)(node->data))[0]);
				free(((int**)(node->data))[1]);
				break;
		}
	}
	free(node);
}

void parser_print_indent(int amt) {
	for(int i = 0; i < amt; ++i) {
		printf("  ");
	}
}

void parser_print_tree(ParseNode* node, int indent) {
	switch(node->type) {
		case N_ROOT:
			parser_print_indent(indent);
			printf("{\n");
			
			if(node->data != NULL) {
				parser_print_indent(indent + 1);
				printf("Function definition: {\n");
				parser_print_tree(node->data, indent + 2);
				parser_print_indent(indent + 1);
				printf("}\n");
			}

			parser_print_indent(indent);
			printf("}\n");
			break;
		case N_FUNC_DEF:
			parser_print_indent(indent);
			printf("Name: %s\n", ((char**)(node->data))[0]);
			parser_print_indent(indent);
			printf("Statement: {\n");
			parser_print_tree(((ParseNode**)(node->data))[1], indent + 1);
			parser_print_indent(indent);
			printf("}\n");
			break;
		case N_STATEMENT:
			parser_print_indent(indent);
			printf("Function call: {\n");
			parser_print_tree(((ParseNode**)(node->data))[0], indent + 1);
			parser_print_indent(indent);
			printf("}\n");
			break;
		case N_FUNC_CALL:
			parser_print_indent(indent);
			printf("Identifier: %s\n", ((char**)(node->data))[0]);
			parser_print_indent(indent);
			printf("Param: %d\n", *(((int**)(node->data))[1]));
			break;
	}

}
