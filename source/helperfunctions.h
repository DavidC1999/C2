#ifndef _HELPERFUNCTIONS_H
#define _HELPERFUNCTIONS_H
#include <assert.h>
#include <stdbool.h>

#ifdef DEBUG
#define d_assert(x) assert(x)
#else
#define d_assert(x)
#endif

bool is_number(char c);
bool is_identifier_char(char c);

#endif  //_HELPERFUNCTIONS_H
