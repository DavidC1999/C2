#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

static void panic(char* message, int line) {
	fprintf(stderr, "Error while interpreting on line %d: %s\n", line, message);
	exit(1);
}

void define_func(char* name, ParseNode* statement) {
	printf("Defining function \"%s\"\n", name);
	(void)statement;
}

void call_func(char* name, int param) {
	printf("Calling function \"%s\" with param %d\n", name, param);
}

void visit_node(ParseNode* node) {
	switch(node->type) {
		case N_ROOT: {
			visit_node(node->data);
			break;
		}
		case N_FUNC_DEF: {
			char* name = ((char**)node->data)[0];
			ParseNode* statement = ((ParseNode**)node->data)[1];
			define_func(name, statement);
			break;
		}
		case N_STATEMENT: {
			ParseNode* func_call = ((ParseNode**)node->data)[0];
			visit_node(func_call);
			break;
		}
		case N_FUNC_CALL: {
			char* name = ((char**)node->data)[0];
			int param = *(((int**)node->data)[1]);
			call_func(name, param);
			break;
		}
	}
}

void interpret(ParseNode* node) {
	if(node->type != N_ROOT) {
		char buffer[100];
		strncpy(buffer, "Interpreting should start at root node", 100);
		panic(buffer, 0);
	}
	visit_node(node);
}
