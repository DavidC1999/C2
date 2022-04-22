#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "parser.h"
#include "hashtable/hashtable.h"

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
	int statement_amt;
	ParseNode* statements;
} UserFunc;

static HashTable* user_functions;

typedef struct BuiltinFunc {
	char name[MAX_FUNCTION_NAME_LEN];
	void (*func)(int);
} BuiltinFunc;

static HashTable* builtin_functions;

static void panic(char* message, int line) {
	fprintf(stderr, "Error while interpreting on line %d: %s\n", line, message);
	exit(1);
}

static void init_funcs() {
	builtin_functions = hashtable_new(ANY_T, MAX_BUILTIN_FUNCTIONS);
	hashtable_set(builtin_functions, "print", builtin_print);
	hashtable_set(builtin_functions, "putc", builtin_putc);

	user_functions = hashtable_new(ANY_T, MAX_USER_FUNCTIONS);

	// we malloc our own user functions to put in the hashtable
	// make sure they get freed when the hashtable is freed
	hashtable_force_free_values(user_functions);
}

static void define_func(char* name, int line, int statement_amt, ParseNode* statements) {
	UserFunc* new_func = malloc(sizeof(UserFunc));
	new_func->statement_amt = statement_amt;
	new_func->statements = statements;

	if(!hashtable_set(user_functions, name, new_func)) {
		char buffer[100];
		snprintf(buffer, 100, "Unable to define function %s", name);
		panic(buffer, line);
	}
}

static bool call_func(char* name, int param) {
	HashEntry buffer;
	if(hashtable_get(user_functions, &buffer, name)) {
		UserFunc* user_func = buffer.value;
		for(int i = 0; i < user_func->statement_amt; ++i) {
			visit_node(&user_func->statements[i]);
		}

		return true;
	}
	
	if(hashtable_get(builtin_functions, &buffer, name)) {
		void (*builtin_func)(int) = buffer.value;
		builtin_func(param);
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

	hashtable_free(builtin_functions);
	hashtable_free(user_functions);
}
