#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <stddef.h>

typedef struct Token {
    int type;
    struct Token* next;
    int line;
    union {
        char* name;  // for identifier
        int number;  // for number or keyword
    };
} Token;

typedef struct TokenLL {
    Token* head;
    Token* tail;
    Token* current;
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
    T_SEMICOLON,
    T_ASSIGN,
    T_PLUS,
    T_MINUS,
    T_ASTERISK,
    T_SLASH,
    T_GREATER,
    T_LESS,
    T_GEQUAL,
    T_LEQUAL,
    T_EQUAL,
};

extern char* token_type_to_name[];

enum KeywordTypes {
    K_FUNC,
    K_VAR,
    K_IF,
    K_WHILE,
};

extern char* keyword_type_to_name[];

void tokenize(char* text, TokenLL** result);
void print_tokens(Token* node);
void free_token(Token* node);

#endif  //_TOKENIZER_H
