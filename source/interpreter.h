#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "parser.h"

typedef void (*builtin_panic_func_t)(char*);

void interpret(ParseNode* node);

#endif  //_INTERPRETER_H
