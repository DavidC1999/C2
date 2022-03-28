#include "helperfunctions.h"

bool is_identifier_char(char c) {
	return (c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			c == '_';
}

