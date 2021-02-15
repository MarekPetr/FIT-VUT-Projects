/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#ifndef SYMBOL_H
#define SYMBOL_H

#define DEBUG
#include "errors.h"
#include "symtable.h"
#include "lex.h"

typedef enum {
	variable_integer,
	variable_double,
	variable_string,
} variable_type;

typedef struct {
	Tstring argument_name;
	variable_type type;
} function_arguments;

typedef struct variable_data {
	int inicialized;
	variable_type type;
	union {
		int value_integer;
		double value_double;
		Tstring value_string;
	} value;
	char *name;
} variable_data;

typedef struct function_data {
	int defined; //predpokláme, že funkcia bola deklarovaná ak existuje položka v globálnej tabuľke
	unsigned arguments_count;
	char return_type;
	function_arguments *arguments;
	htab_t *local_symbol_table;
	char *name;
} function_data;

// Tvorba dát: variable *data = create_data_variable(token) - vracia NULL, ak zlyhá malloc
void set_type_variable(variable_data *data, Ttoken *token);
void set_value_variable(variable_data *data, Ttoken *token);

// Volane priamo len v prípade, že ešte nebola volana funkcia variable_data_to_table a je potreba uvoľniť pamäť
void free_data_variable(variable_data *data);

void set_return_type_function(function_data *data, Ttoken *token);

// Ret ERR_INTERN v pripade ze zlyhá malloc/realloc, inak 0
int add_argument_function(function_data *data, Ttoken *token);
void set_argument_type_function(function_data *data, Ttoken *token);

// Volane priamo len v pripade, že ešte nebola volana funcia function_data_to_table a je potreba uvoľniť pamäť
void free_data_function(function_data *data);

//Kontrola či premenná existuje v lokálnej tabuľke globálnych dát, 1 ak existuje, 0 ak neexistuje alebo tabuľka v glob. datách == NULL
int variable_exist(char *variable_name);

int check_variable_type(char *variable_name, Tstate state);
Tstate return_variable_type(char *variable_name);
Tstate return_function_type(void);
int check_variable_inicialized(char *variable_name);


//Použiť v rekurzívnom zostupe
int check_function_return_type(Tstate state);
int check_argument_count(unsigned count);
int check_argument_type(Tstate state, unsigned index);
int check_argument_name(char *name, unsigned index);
int check_defined_function(void);

int buildin_to_table(void);

void variable_init(htab_listitem *item, char *variable_name);
void function_init(htab_listitem *item, char *function_name);

#endif
