#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "helperfunctions.h"

#define MAX_IDENTIFIER_LENGTH 1000

enum Tokenypes {
	T_HEAD,
	T_KEYWORD,
	T_LPAREN,
	T_RPAREN,
	T_LBRACE,
	T_RBRACE,
	T_SEMICOLON,
	T_IDENTIFIER
};

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

void tokenizer_get_identifier(char** text, Token* node) {
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

void tokenize(char* text, Token** result) {
	Token* head = (Token*)malloc(sizeof(Token));
	head->type = T_HEAD;
	head->next = NULL;

	while(*text != EOF) {
		if(is_identifier_char(*text)) {
			tokenizer_get_identifier(&text, head);
		} else if (*text == '\n') {
			++text;
		}
	}
	*result = head;
}


void read_file(FILE* file, size_t n, char* buffer) {
	char c;
	uint32_t i = 0;
	while((c = getc(file)) != EOF) {
		if(i >= n) return;
		buffer[i++] = c;
	}
	buffer[i] = EOF;
}

size_t get_file_size(FILE* file) {
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	return size;
}

int main(void) {
	FILE* file = fopen("testfile.prog", "r");
	size_t file_size = get_file_size(file);
	char* buffer = (char*)malloc(sizeof(char) * file_size);
	
	read_file(file, file_size, buffer);

	fclose(file);
	
	Token* tokens;

	tokenize(buffer, &tokens);
	
	print_tokens(tokens);

	free(buffer);

	return 0;
}
