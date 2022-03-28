#include <stdio.h>
#include <stdlib.h>

void read_file(FILE* file, size_t n, char* buffer) {
	char c;
	while((c = getc(file)) != EOF) {
		putchar(c);
	}
}

size_t get_file_size(FILE* file) {
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	return size;
}

int main(void) {
	FILE* file = fopen("program.prog", "r");
	size_t file_size = get_file_size(file);
	char* buffer = (char*)malloc(sizeof(char) * file_size);
	
	read_file(file, file_size, buffer);

	fclose(file);

	printf("%s", buffer);
	free(buffer);

	return 0;
}
