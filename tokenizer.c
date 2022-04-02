#define MAX_IDENTIFIER_LENGTH 1000

#include <string.h>

#include "helperfunctions.h"

enum TokenTypes {
	T_HEAD,
	T_KEYWORD,
	T_LPAREN,
	T_RPAREN,
	T_LBRACE,
	T_RBRACE,
	T_SEMICOLON,
	T_IDENTIFIER
};

enum KeywordTypes {
	K_FUNC
}

typedef struct Token {
	int type;
	struct Token* next;
	void* data;
} Token;

void append_token(Token* prev_tail, int new_type, void* data) {
	Token* new_token = (Token*)malloc(sizeof(Token));
	new_token->type = new_type;
	new_token->next = NULL;
	new_token->data = data;
	
	prev_tail->next = new_token;
}

void free_token(Token* node) {
	if(node == NULL) return;
	free_token(node->next);
	free(node->data);
	free(node);
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
	}
	print_tokens(node->next);
}

void tokenizer_create_identifier(char** text, Token* node) {
	char buffer[MAX_IDENTIFIER_LENGTH] = {0};
	size_t buffer_i = 0;
	while(is_identifier_char(**text)) {
		if(buffer_i >= MAX_IDENTIFIER_LENGTH - 2) break;
		buffer[buffer_i++] = **text;
		++*text;
	}
	buffer[buffer_i] = '\0';

	char* data = (char*)malloc(sizeof(char) * buffer_i);
	strncpy(data, buffer, buffer_i);

	append_token(node, T_IDENTIFIER, data);
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

void tokenize(char* text, Token** result) {
	Token* head = (Token*)malloc(sizeof(Token));
	head->type = T_HEAD;
	head->next = NULL;

	while(*text != EOF) {
		if(is_identifier_char(*text)) {
			tokenizer_create_identifier(&text, head);
			head = head->next;
			tokenizer_check_identifier_is_keyword(head);
		} else if (*text == '\n') {
			++text;
		}
	}
	*result = head;
}
