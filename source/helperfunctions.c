#include "helperfunctions.h"

bool is_number(char c) {
    return c >= '0' && c <= '9';
}

bool is_identifier_char(char c) {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           c == '_';
}
