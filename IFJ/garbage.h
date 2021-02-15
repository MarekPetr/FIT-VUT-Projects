/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#ifndef GARBAGE_H
#define GARBAGE_H

#include <stdio.h>
#include <stdlib.h>

#include "symbol.h"
#include "symtable.h"

struct alloc_item {
	void *ptr;
	struct alloc_item *next;
};

struct global_alloc {
	struct alloc_item *allocated;
	struct alloc_item *allocated_last;

	htab_t *global_table;
	char *current_func_name;
	htab_t *local_sym;
	function_arguments *current_arguments;
};

extern struct global_alloc global;

void global_init(void);
void global_free(void);

void *g_malloc(unsigned long size);
void *g_realloc(void *ptr, unsigned long size);

#define g_free(_ptr)										\
do {														\
	struct alloc_item *tmp = global.allocated;				\
	struct alloc_item *prev = tmp;							\
	while(tmp != NULL) {									\
		if(tmp->ptr == _ptr) {								\
			if(prev != tmp && tmp->next != NULL) {			\
				prev->next = tmp->next;						\
			} else if(prev == tmp && tmp->next != NULL) {	\
				global.allocated = tmp->next;				\
			} if(tmp->next == NULL)	{						\
				global.allocated_last = prev;				\
				prev->next = NULL;							\
			}												\
															\
			if(prev == tmp && tmp->next == NULL) {			\
				tmp->ptr = NULL;							\
			} else {										\
				free(tmp);									\
			}												\
			break;											\
		}													\
															\
		prev = tmp;											\
		tmp = tmp->next;									\
	}														\
															\
	if(_ptr != NULL) {										\
		free(_ptr);											\
		_ptr = NULL;										\
	}														\
} while(0);

#endif