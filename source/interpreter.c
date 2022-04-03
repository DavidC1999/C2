#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "parser.h"

#define MAX_USER_FUNCTIONS 100
#define MAX_BUILTIN_FUNCTIONS 100
#define MAX_FUNCTION_NAME_LEN 100

static void visit_node(ParseNode* node);

void builtin_print(int param) {
	printf("%d\n", param);
}

typedef struct UserFunc {
	bool taken;
	char name[MAX_FUNCTION_NAME_LEN];
	ParseNode* statement;
} UserFunc;

static UserFunc user_functions[MAX_USER_FUNCTIONS];

typedef struct BuiltinFunc {
	char name[MAX_FUNCTION_NAME_LEN];
	void (*func)(int);
} BuiltinFunc;

static BuiltinFunc builtin_functions[MAX_BUILTIN_FUNCTIONS];

static void panic(char* message, int line) {
	fprintf(stderr, "Error while interpreting on line %d: %s\n", line, message);
	exit(1);
}

static void init_funcs() {
	for(int i = 0; i < MAX_USER_FUNCTIONS; ++i) {
		user_functions[i].taken = false;
	}
	
	strncpy(builtin_functions[0].name, "print", MAX_FUNCTION_NAME_LEN);
	builtin_functions[0].func = builtin_print;
}

static void define_func(char* name, ParseNode* statement) {
	// printf("Defining function \"%s\"\n", name);

	// Find first free spot in user_functions array:
	int i;
	bool found_empty_spot = false;
	for(i = 0; i < MAX_USER_FUNCTIONS; ++i) {
		if(user_functions[i].taken == false) {
			found_empty_spot = true;
			break;
		}
	}

	if(!found_empty_spot) {
		panic("Too many function definitions", statement->line);
	}

	user_functions[i].taken = true;

	strncpy(user_functions[i].name, name, MAX_FUNCTION_NAME_LEN);
	user_functions[i].statement = statement;
}

static bool call_func(char* name, int param) {
	// printf("Calling function \"%s\" with param %d\n", name, param);
	int i;
	bool found_user_func = false;
	for(i = 0; i < MAX_USER_FUNCTIONS; ++i) {
		if(strcmp(user_functions[i].name, name) == 0) {
			found_user_func = true;
			break;
		}
	}

	if(found_user_func) {
		visit_node(user_functions[i].statement);
		return true;
	}
	
	bool found_builtin_func = false;
	for(i = 0; i < MAX_USER_FUNCTIONS; ++i) {
		if(strcmp(builtin_functions[i].name, name) == 0) {
			found_builtin_func = true;
			break;
		}
	}

	if(found_builtin_func) {
		builtin_functions[i].func(param);
		return true;
	}

	return false;
}

static void visit_node(ParseNode* node) {
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
			bool success = call_func(name, param);
			if(!success) {
				char buffer[100];
				snprintf(buffer, 100, "Unknown function: %s", name);
				panic(buffer, node->line);
			}
			break;
		}
	}
}

void interpret(ParseNode* node) {

	init_funcs();
	if(node->type != N_ROOT) {
		char buffer[100];
		strncpy(buffer, "Interpreting should start at root node", 100);
		panic(buffer, 0);
	}

	if(node->data != NULL) {
		visit_node(node);
	}

	bool success = call_func("main", 0);
	if(!success) {
		panic("Every program should have a main function", 0);
	}
}
