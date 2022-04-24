#define MAX_IDENTIFIER_LENGTH 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"
#include "helperfunctions.h"

static int curr_line = 1;

char* token_type_to_name[] = {
	"T_HEAD",
	"T_NUMBER",
	"T_IDENTIFIER",
	"T_KEYWORD",
	"T_LPAREN",
	"T_RPAREN",
	"T_LBRACE",
	"T_RBRACE",
	"T_SEMICOLON",
	"T_EQUAL"
};

char* keyword_type_to_name[] = {
	"K_FUNC",
	"K_VAR",
};

static void panic(char* message) {
	fprintf(stderr, "Error while tokenizing on line %d: %s\n", curr_line, message);
	exit(1);
}

void append_token(TokenLL* tokens, int new_type, void* data) {
	Token* new_token = (Token*)malloc(sizeof(Token));
	new_token->type = new_type;
	new_token->next = NULL;
	new_token->line = curr_line;
	switch(new_type) {
		case T_NUMBER:
		case T_KEYWORD:
			new_token->number = *(int*)data;
			break;
		case T_IDENTIFIER:
			new_token->name = (char*)data;
			break;
	}
	
	tokens->tail->next = new_token;
	tokens->tail = new_token;
}

void free_token(Token* node) {
	if(node == NULL) return;
	free_token(node->next);
	if(node->type == T_IDENTIFIER) {
		free(node->name);
	}
	free(node);
}

void tokenizer_create_number(char** text, TokenLL* tokens) {
	char buffer[MAX_IDENTIFIER_LENGTH] = {0};
	size_t buffer_i = 0;
	while(is_number(**text)) {
		if(buffer_i >= MAX_IDENTIFIER_LENGTH - 2) break;
		buffer[buffer_i++] = **text;
		++*text;
	}
	buffer[buffer_i] = '\0';

	int result = atoi(buffer);

	append_token(tokens, T_NUMBER, &result);
}

void tokenizer_create_identifier(char** text, TokenLL* tokens) {
	char buffer[MAX_IDENTIFIER_LENGTH] = {0};
	size_t buffer_i = 0;
	while(is_identifier_char(**text)) {
		if(buffer_i >= MAX_IDENTIFIER_LENGTH - 2) break;
		buffer[buffer_i++] = **text;
		++*text;
	}
	buffer[buffer_i] = '\0';

	char* data = (char*)malloc(sizeof(char) * (buffer_i + 1));
	strncpy(data, buffer, buffer_i + 1);
	data[buffer_i] = '\0';

	append_token(tokens, T_IDENTIFIER, data);
}

void tokenizer_check_identifier_is_keyword(Token* token) {
	if(token->type != T_IDENTIFIER) return;
	
	if(strcmp(token->name, "func") == 0) {
		free(token->name);
		token->type = T_KEYWORD;
		token->number = K_FUNC;
	} else if(strcmp(token->name, "var") == 0) {
		free(token->name);
		token->type = T_KEYWORD;
		token->number = K_VAR;
	}
}

bool tokenizer_should_skip(char c) {
	switch(c) {
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return true;
		default:
			return false;
	}
}

void tokenize(char* text, TokenLL** result) {
	*result = (TokenLL*)malloc(sizeof(TokenLL));

	Token* head = (Token*)malloc(sizeof(Token));
	head->type = T_HEAD;
	head->next = NULL;

	(*result)->head = head;
	(*result)->tail = head;

	while(*text != EOF) {
		if(is_number(*text)) {
			tokenizer_create_number(&text, *result);
		} else if(is_identifier_char(*text)) {
			tokenizer_create_identifier(&text, *result);
			tokenizer_check_identifier_is_keyword((*result)->tail);
		} else if(*text == '(') {
			append_token(*result, T_LPAREN, NULL);
			++text;
		} else if(*text == ')') {
			append_token(*result, T_RPAREN, NULL);
			++text;
		} else if(*text == '{') {
			append_token(*result, T_LBRACE, NULL);
			++text;
		} else if(*text == '}') {
			append_token(*result, T_RBRACE, NULL);
			++text;
		} else if(*text == ';') {
			append_token(*result, T_SEMICOLON, NULL);
			++text;
		} else if(*text == '=') {
			append_token(*result, T_EQUAL, NULL);
			++text;
		} else if(*text == '/') {
			++text;
			if(*text == EOF) break;
			if(*text == '/') {
				++text;
				while(*text != '\n' && *text != EOF) {
					++text;
				}
			} else {
				char buffer[100];
				snprintf(buffer, 100, "Expected '/', but found '%c' instead", *text);
				panic(buffer);
			}
		} else if (tokenizer_should_skip(*text)) {
			if(*text == '\n') ++curr_line;
			++text;
		} else {
			char buffer[100];
			snprintf(buffer, 100, "Unexpected char: '%c'", *text);
			panic(buffer);
		}
	}

	(*result)->head = head->next;
	(*result)->current = (*result)->head;
	free(head);
}

void print_tokens(Token* node) {
	if(node == NULL) {
		printf("\n");
		return;
	}

	switch(node->type) {
		case T_HEAD:
			printf("HEAD -> ");
			break;
		case T_IDENTIFIER:
			printf("IDENTIFIER: %s -> ", node->name);
			break;
		case T_KEYWORD:
			printf("KEYWORD: %s -> ", keyword_type_to_name[node->number]);
			break;
		case T_NUMBER:
			printf("NUMBER: %d -> ", node->number);
			break;
		case T_LPAREN:
			printf("'(' -> ");
			break;
		case T_RPAREN:
			printf("')' -> ");
			break;
		case T_LBRACE:
			printf("'{' -> ");
			break;
		case T_RBRACE:
			printf("'}' -> ");
			break;
		case T_SEMICOLON:
			printf("';' -> ");
			break;
		case T_EQUAL:
			printf("'=' -> ");
			break;
	}
	print_tokens(node->next);
}

