#ifndef _TOKENIZER_H
#define _TOKENIZER_H

typedef struct Token {
	int type;
	struct Token* next;
	void* data;
} Token;

typedef struct TokenLL {
	Token* head;
	Token* tail;
} TokenLL;

enum TokenTypes {
	T_HEAD,
	T_NUMBER,
	T_IDENTIFIER,
	T_KEYWORD,
	T_LPAREN,
	T_RPAREN,
	T_LBRACE,
	T_RBRACE,
	T_SEMICOLON
};

enum KeywordTypes {
	K_FUNC
};

void tokenize(char* text, TokenLL** result);
void print_tokens(Token* node);
void free_token(Token* node);

#endif //_TOKENIZER_H
