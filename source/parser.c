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

ParseNode* parser_get_statement(TokenLL* tokens) {
	parser_expect_token_type(tokens->current, T_IDENTIFIER);

	int line = tokens->current->line;

	size_t identifier_len = strlen(tokens->current->name) + 1; // includes '\0'
	char* identifier = malloc(sizeof(char) * (identifier_len));
	strncpy(identifier, tokens->current->name , identifier_len);

	parser_advance_token(tokens);

	switch(tokens->current->type) {
		case T_LPAREN: {
			parser_advance_token(tokens);

			bool param_is_var = false;

			int param = 0;
			if(tokens->current->type == T_NUMBER) {
				param = tokens->current->number;
				parser_advance_token(tokens);
			}

			char* var_name;
			if(tokens->current->type == T_IDENTIFIER) {
				param_is_var = true;
				var_name = tokens->current->name;
				parser_advance_token(tokens);
			}

			parser_expect_token_type(tokens->current, T_RPAREN);
			parser_advance_token(tokens);

			ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
			result->type = N_FUNC_CALL;
			result->line = line;
			result->func_call_params.name = identifier;
			result->func_call_params.param_is_var = param_is_var;
			if(param_is_var) {
				size_t str_len = strlen(var_name) + 1; // including '\0'
				result->func_call_params.var_name = malloc(sizeof(char) * str_len);
				strncpy(result->func_call_params.var_name, var_name, str_len);
			} else {
				result->func_call_params.param = param;
			}

			parser_expect_token_type(tokens->current, T_SEMICOLON);
			parser_advance_token(tokens);

			return result;
		}
		case T_EQUAL: {
			parser_advance_token(tokens);
			parser_expect_token_type(tokens->current, T_NUMBER);
			ParseNode* result = (ParseNode*)malloc(sizeof(ParseNode));
			result->type = N_VAR_ASSIGN;
			result->line = line;
			result->assign_params.name = identifier;
			result->assign_params.value = tokens->current->number;
			parser_advance_token(tokens);
			
			parser_expect_token_type(tokens->current, T_SEMICOLON);
			parser_advance_token(tokens);

			return result;
		}
	}
	panic("invalid statement", line);
	
	return NULL;
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
	result->func_def_params.statements = malloc(sizeof(ParseNode*) * statement_counter);


	for(int i = 0; i < statement_counter; ++i) {
		result->func_def_params.statements[i] = statements[i];
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
			free(node->func_def_params.name);

			for(int i = 0; i < node->func_def_params.statement_amt; ++i) {
				free(node->func_def_params.statements[i]);
			}

			free(node->func_def_params.statements);

			break;
		case N_VAR_DEF:
			free(node->var_def_params.name);
			break;
		case N_FUNC_CALL:
			if(node->func_call_params.param_is_var) {
				free(node->func_call_params.var_name);
			}

			free(node->func_call_params.name);
			break;
		case N_VAR_ASSIGN:
			free(node->assign_params.name);
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
				parser_print_AST(node->func_def_params.statements[i], indent + 1);
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
			if(node->func_call_params.param_is_var)
				printf("Param: %s\n", node->func_call_params.var_name);
			else
				printf("Param: %d\n", node->func_call_params.param);

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
			printf("Value: %d\n", node->assign_params.value);

			print_indent(indent);
			printf("}\n");
			break;
		}
	}

}
