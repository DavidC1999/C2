#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "tokenizer.h"

void parser_advance_token(TokenLL* tokens) {
	if(tokens->current == NULL) return;
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

	if(token->number != expected_keyword) {
		char buffer[100];
		snprintf(buffer, 100, "Expected keyword of type %s, but found %s, instead", token_type_to_name[token->number], token_type_to_name[expected_keyword]);
		panic(buffer, token->line);
	
	}
}

ParseNode* parser_get_function_call(TokenLL* tokens) {
	parser_expect_token_type(tokens->current, T_IDENTIFIER);
	int line = tokens->current->line;

	size_t name_len = strlen(tokens->current->name);
	char* name = (char*)malloc(sizeof(char) * (name_len + 1));
	strncpy(name, tokens->current->name, name_len + 1);
	parser_advance_token(tokens);

	parser_expect_token_type(tokens->current, T_LPAREN);
	parser_advance_token(tokens);

	int param = 0;
	if(tokens->current->type == T_NUMBER) {
		param = tokens->current->number;
		parser_advance_token(tokens);
	}

	parser_expect_token_type(tokens->current, T_RPAREN);
	parser_advance_token(tokens);

	ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
	result->type = N_FUNC_CALL;
	result->line = line;
	result->func_call_params.name = name;
	result->func_call_params.param = param;

	return result;
}

ParseNode* parser_get_statement(TokenLL* tokens) {
	ParseNode* func_call = parser_get_function_call(tokens);
	parser_expect_token_type(tokens->current, T_SEMICOLON);
	parser_advance_token(tokens);

	ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
	result->type = N_STATEMENT;
	result->line = func_call->line;
	result->statement_params.function_call = func_call;

	return result;
}

ParseNode* parser_get_function_definition(TokenLL* tokens) {
	parser_expect_keyword(tokens->current, K_FUNC);
	int line = tokens->current->line;
	parser_advance_token(tokens);
	
	parser_expect_token_type(tokens->current, T_IDENTIFIER);
	size_t identifier_len = strlen(tokens->current->name);
	char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
	strncpy(identifier_name, tokens->current->name, identifier_len);
	identifier_name[identifier_len] = '\0';
	parser_advance_token(tokens);

	parser_expect_token_type(tokens->current, T_LPAREN);
	parser_advance_token(tokens);
	parser_expect_token_type(tokens->current, T_RPAREN);
	parser_advance_token(tokens);
	parser_expect_token_type(tokens->current, T_LBRACE);
	parser_advance_token(tokens);

	ParseNode* statements[MAX_STATEMENTS_PER_FUNC];
	int statement_counter = 0;
	while(tokens->current->type == T_IDENTIFIER) {
		if(statement_counter >= MAX_STATEMENTS_PER_FUNC) {
			panic("Too many statements for one function", tokens->current->line);
		}

		statements[statement_counter++] = parser_get_statement(tokens);
	}

	parser_expect_token_type(tokens->current, T_RBRACE);
	parser_advance_token(tokens);

	ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
	result->type = N_FUNC_DEF;
	result->line = line;
	result->func_def_params.name = identifier_name;
	result->func_def_params.statement_amt = statement_counter;
	result->func_def_params.statements = (ParseNode*)malloc(sizeof(ParseNode) * statement_counter);

	for(int i = 0; i < statement_counter; ++i) {
		result->func_def_params.statements[i].type = statements[i]->type;
		result->func_def_params.statements[i].line = statements[i]->line;
		result->func_def_params.statements[i].statement_params.function_call = statements[i]->statement_params.function_call;
		free(statements[i]);
	}

	return result;
}

ParseNode* parser_get_variable_definition(TokenLL* tokens) {
	parser_expect_keyword(tokens->current, K_VAR);
	int line = tokens->current->line;
	parser_advance_token(tokens);

	parser_expect_token_type(tokens->current, T_IDENTIFIER);
	size_t identifier_len = strlen(tokens->current->name);
	char* identifier_name = (char*)malloc(sizeof(char) * (identifier_len + 1));
	strncpy(identifier_name, tokens->current->name, identifier_len);
	identifier_name[identifier_len] = '\0';
	parser_advance_token(tokens);

	parser_expect_token_type(tokens->current, T_SEMICOLON);
	parser_advance_token(tokens);

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
	while(tokens->current != NULL &&
			tokens->current->type == T_KEYWORD) {
		switch(tokens->current->number) {
			case K_FUNC:
				definitions[definitions_counter++] = parser_get_function_definition(tokens);
				break;
			case K_VAR:
				definitions[definitions_counter++] = parser_get_variable_definition(tokens);
				break;
		}
	}

	if(tokens->current != NULL) {
		char buffer[100];
		snprintf(buffer, 100, "Unexpected token: %s", token_type_to_name[tokens->current->type]);
		panic(buffer, tokens->current->line);
	}

	ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
	result->type = N_ROOT;
	result->line = 0;
	result->root_params.count = definitions_counter;
	result->root_params.definitions = (ParseNode*)malloc(sizeof(ParseNode) * definitions_counter);

	for(int i = 0; i < definitions_counter; ++i) {
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
	switch(node->type) {
		case N_ROOT:
			free(node->root_params.definitions);
			break;
		case N_FUNC_DEF:
			int statment_amt = node->func_def_params.statement_amt;
			free(node->func_def_params.name);

			for(int i = 0; i < statment_amt; ++i) {
				ParseNode* to_free = node->func_def_params.statements[i].statement_params.function_call;
				free_AST(to_free);
			}
			free(node->func_def_params.statements);

			break;
		case N_VAR_DEF:
			free(node->var_def_params.name);
			break;
		case N_STATEMENT:
			free_AST(node->statement_params.function_call);
			break;
		case N_FUNC_CALL:
			free(node->func_call_params.name);
			break;
	}
	free(node);
}

static void print_indent(int amt) {
	for(int i = 0; i < amt; ++i) {
		printf("  ");
	}
}

void parser_print_AST(ParseNode* node, int indent) {
	switch(node->type) {
		case N_ROOT: {
			print_indent(indent);
			printf("[\n");

			int def_amt = node->root_params.count;
			ParseNode* definitions = node->root_params.definitions;
			
			for(int i = 0; i < def_amt; ++i) {
				print_indent(indent + 1);
				if(definitions[i].type == N_FUNC_DEF)
					printf("Function definition: {\n");
				else if(definitions[i].type == N_VAR_DEF)
					printf("Variable definition: {\n");
				parser_print_AST(&definitions[i], indent + 2);
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
			for(int i = 0; i < statement_amt; ++i) {
				parser_print_AST(&node->func_def_params.statements[i], indent + 1);
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
		case N_STATEMENT: {
			print_indent(indent);
			printf("Function call: {\n");
			parser_print_AST(node->statement_params.function_call, indent + 1);
			print_indent(indent);
			printf("}\n");
			break;
		}
		case N_FUNC_CALL: {
			print_indent(indent);
			printf("Identifier: %s\n", node->func_call_params.name);
			print_indent(indent);
			printf("Param: %d\n", node->func_call_params.param);
			break;
		}
	}

}
