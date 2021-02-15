/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#include "symbol.h"

#ifndef SYMTABLE_H
#define SYMTABLE_H

#define DEBUG
#include "errors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

typedef enum {
	type_variable,
	type_function,
} item_type;

typedef struct htab_listitem {
	struct htab_listitem *next;
	item_type type;
	union
	{
		struct variable_data *variable;
		struct function_data *function;
	} pointer;
	char *key;
} htab_listitem;

typedef struct htab_t {
	unsigned size;
	struct htab_listitem *list[];
} htab_t;


/* Najpodstatnejsie funkcie:
 * 	Tvorba tabuľky: htab_t *table = htab_init(size) - vracia NULL ak zlyhá malloc
 *	htab_free(table) uvolni postupne vsetky polozky tabulky a samotnu tabulku
 */
htab_t *htab_init(unsigned size);
void htab_free(htab_t *table);
htab_listitem *htab_find(htab_t *table, char *key); //vracia NULL ak položku nenašlo


// Priamo v syntaktickej/semantickej nepotrebne, sú volané z funkcii TS
unsigned hash_function(const char *str);
htab_listitem *htab_lookup_add(htab_t *table, char *key);
void htab_foreach(htab_t *table, void (*function)(const char *key, item_type type, htab_listitem *item));
void htab_remove(htab_t *table, const char *key);
void htab_clear(htab_t *table);

#endif
