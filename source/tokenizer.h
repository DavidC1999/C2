#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <stddef.h>

#define MAX_IDENTIFIER_LENGTH 1000

typedef struct Token {
    int type;
    struct Token* next;
    int line;
    union {
        char* string;  // for identifier and string literals
        int number;    // for number or keyword
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
    T_LSQUARE,
    T_RSQUARE,
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
    T_COMMA,
    T_AMPERSAND,
    T_AT,
    T_PIPE,
    T_DBL_GREATER,
    T_DBL_LESS,
    T_STRING,
};

extern char* token_type_to_name[];

enum KeywordTypes {
    K_FUNC,
    K_VAR,
    K_IF,
    K_WHILE,
    K_RETURN,
#ifdef DEBUG
    K_DEBUG,
#endif
};

extern char* keyword_type_to_name[];

void tokenize(char* text, TokenLL** result);
void print_tokens(Token* node);
void free_token(Token* node);

#endif  //_TOKENIZER_H
