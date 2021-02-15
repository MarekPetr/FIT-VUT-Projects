/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#include "garbage.h"

#define HTAB_SIZE 23

struct global_alloc global;

void *g_malloc(unsigned long size)
{
	if(global.allocated->ptr != NULL || global.allocated != global.allocated_last) {
		global.allocated_last->next = malloc(sizeof(struct alloc_item));
		global.allocated_last = global.allocated_last->next;
	}

	global.allocated_last->ptr = malloc(size);
	global.allocated_last->next = NULL;

	return global.allocated_last->ptr;
}

void *g_realloc(void *ptr, unsigned long size)
{
	struct alloc_item* temp = global.allocated;

	while(temp != NULL) {
		if(temp->ptr == ptr) {
			temp->ptr = realloc(temp->ptr, size);

			return temp->ptr;
		}
		temp = temp->next;
	}
	
	return NULL;
}

void global_init(void)
{
	global.allocated = malloc(sizeof(struct alloc_item));
	global.allocated_last = global.allocated;
	global.allocated->ptr = NULL;

	global.global_table = htab_init(HTAB_SIZE);
	global.local_sym = NULL;

	global.current_arguments = NULL;
}

void global_free(void)
{
	struct alloc_item *tmp;
	struct alloc_item *ptr;

	ptr = global.allocated;

	while(ptr != NULL)
	{
		if(ptr->ptr != NULL)
		{
			free(ptr->ptr);
		}
		
		tmp = ptr;
		ptr = ptr->next;
		free(tmp);
	}
}