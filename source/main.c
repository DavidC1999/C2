#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "tokenizer.h"
#include "parser.h"


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

int main(int argc, char** argv) {
	if(argc != 2) {
		fprintf(stderr, "Please specify file\n");
		exit(1);
	}


	FILE* file = fopen(argv[1], "r");
	size_t file_size = get_file_size(file);
	char* buffer = (char*)malloc(sizeof(char) * file_size);
	
	read_file(file, file_size, buffer);

	fclose(file);
	
	TokenLL* tokens;

	tokenize(buffer, &tokens);
	
	print_tokens(tokens->head);

	ParseNode* tree = parse(tokens);

	parser_print_tree(tree, 0);

	free_AST(tree);

	free_token(tokens->head);

	free(buffer);

	return 0;
}
