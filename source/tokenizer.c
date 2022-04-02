#define MAX_IDENTIFIER_LENGTH 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"
#include "helperfunctions.h"

void append_token(TokenLL* tokens, int new_type, void* data) {
	Token* new_token = (Token*)malloc(sizeof(Token));
	new_token->type = new_type;
	new_token->next = NULL;
	new_token->data = data;
	
	tokens->tail->next = new_token;
	tokens->tail = new_token;
}

void free_token(Token* node) {
	if(node == NULL) return;
	free_token(node->next);
	free(node->data);
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

	int* data = (int*)malloc(sizeof(int));
	*data = atoi(buffer);

	append_token(tokens, T_NUMBER, data);
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

void tokenizer_check_identifier_is_keyword(Token* node) {
	if(node->type != T_IDENTIFIER) return;
	
	if(strcmp(node->data, "func") == 0) {
		node->type = T_KEYWORD;
		free(node->data);
		int* new_data = (int*)malloc(sizeof(int));
		*new_data = K_FUNC;
		node->data = new_data;
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
		} else if (tokenizer_should_skip(*text)) {
			++text;
		} else {
			fprintf(stderr, "Unexpected char: %c\n", *text);
			exit(1);
		}
	}

	(*result)->head = head->next;
	free(head);
}

void print_keyword(int keyword_ident) {
	switch(keyword_ident) {
		case K_FUNC:
			printf("func");
			break;
		default:
			printf("unknown");
			break;
	}
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
			printf("IDENTIFIER: %s -> ", (char*)node->data);
			break;
		case T_KEYWORD:
			printf("KEYWORD: ");
			print_keyword(*((int*)(node->data)));
			printf(" -> ");
			break;
		case T_NUMBER:
			printf("NUMBER: %d -> ", *(int*)node->data);
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
	}
	print_tokens(node->next);
}

