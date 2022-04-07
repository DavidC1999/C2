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

void builtin_putc(int param) {
	printf("%c", (char)param);
}

typedef struct UserFunc {
	bool taken;
	char name[MAX_FUNCTION_NAME_LEN];
	int statement_amt;
	ParseNode* statements;
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
	
	strncpy(builtin_functions[1].name, "putc", MAX_FUNCTION_NAME_LEN);
	builtin_functions[1].func = builtin_putc;
}

static void define_func(char* name, int line, int statement_amt, ParseNode* statements) {
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
		panic("Too many function definitions", line);
	}

	user_functions[i].taken = true;

	strncpy(user_functions[i].name, name, strlen(name) + 1);
	user_functions[i].statement_amt = statement_amt;
	user_functions[i].statements = statements;
}

static bool call_func(char* name, int param) {
	int i;
	bool found_user_func = false;
	for(i = 0; i < MAX_USER_FUNCTIONS; ++i) {
		if(strcmp(user_functions[i].name, name) == 0) {
			found_user_func = true;
			break;
		}
	}

	if(found_user_func) {
		for(int j = 0; j < user_functions[i].statement_amt; ++j)
			visit_node(&user_functions[i].statements[j]);
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
		case N_FUNC_DEF: {
			char* name = node->func_def_params.name;
			int statement_amt = node->func_def_params.statement_amt;

			ParseNode* statements = node->func_def_params.statements;

			define_func(name, node->line, statement_amt, statements);
			break;
		}
		case N_STATEMENT: {
			ParseNode* func_call = node->statement_params.function_call;
			visit_node(func_call);
			break;
		}
		case N_FUNC_CALL: {
			char* name = node->func_call_params.name;
			int param = node->func_call_params.param;
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
	
	int function_amt = node->root_params.count;
	for(int i = 0; i < function_amt; ++i) {
		ParseNode* funcs = node->root_params.function_definitions;
		visit_node(&funcs[i]);
	}

	bool success = call_func("main", 0);
	if(!success) {
		panic("Every program should have a main function", 0);
	}
}
