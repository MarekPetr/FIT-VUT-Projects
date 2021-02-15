/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#include "symtable.h"
#include "garbage.h"
#include "error.h"

unsigned int hash_function(const char *str)
{
	unsigned int h = 0;
    const unsigned char *p;

    for(p = (const unsigned char*)str; *p != '\0'; p++) {
        h = 65599 * h + *p;
    }	

    return h;
}

htab_t *htab_init(unsigned size)
{
	htab_t *table;

	table = (htab_t *) g_malloc(sizeof(htab_t) + size * sizeof(htab_listitem));
	if(table == NULL) { 
		fprintf(stderr, "Error allocating memory for hash table.");
		return NULL;
	}
	
	table->size = size;

	for(unsigned i = 0; i < size; i++) {
		table->list[i] = NULL;
	}

	return table;
}

htab_listitem *htab_find(htab_t *table, char *key)
{
	unsigned index = hash_function(key) % table->size;
	htab_listitem *item = table->list[index];

	while(item != NULL) {
		if(strcmp(key, item->key) == 0) {
			return item;
		}

		if(item->next != NULL) {
			item = item->next;
		} else {
			break;
		}
	}

	return NULL;
}

htab_listitem *htab_lookup_add(htab_t *table, char *key)
{
	unsigned index = hash_function(key) % table->size;
	htab_listitem *item = table->list[index];

	while(item != NULL) {
		if(strcmp(key, item->key) == 0) {
			return item;
		}

		if(item->next != NULL) {
			item = item->next;
		} else {
			break;
		}
	}

	htab_listitem *new_item = NULL;

	new_item = g_malloc(sizeof(htab_listitem));
	if(new_item == NULL) {
		fprintf(stderr, "Error allocating memory for htab_listitem.");

		return NULL;
	}

	new_item->key = g_malloc(sizeof(char *) * (strlen(key) + 1));
	if(new_item->key == NULL) {
		fprintf(stderr, "Error allocating memory for htab_listitem->key.");
		g_free(new_item);

		return NULL;
	}

	new_item->next = NULL;
	strcpy(new_item->key, key);

	if(item == NULL) {
		table->list[index] = new_item;
	} else {
		item->next = new_item;
	}

	return new_item;
}

void htab_foreach(htab_t *table, void (*function)(const char *key, item_type type, htab_listitem *item))
{
	htab_listitem *item = NULL;

	for(unsigned i = 0; i < table->size; i++) {
		item = table->list[i];

		while(item != NULL) {
			function(item->key, item->type, item);
			item = item->next;
		}
	}
}

void htab_remove(htab_t *table, const char *key)
{
	unsigned index = hash_function(key) % table->size;
	htab_listitem *item = table->list[index];
	htab_listitem *previous_item = NULL;

	while(item != NULL) {
		if(strcmp(key, item->key) == 0) {
			if(previous_item != NULL && item->next != NULL) {
				previous_item = item->next;
			} else if(previous_item == NULL && item->next != NULL) {
				table->list[index] = item->next;
			} else if(previous_item != NULL && item->next == NULL) {
				previous_item = NULL;
			} else {
				table->list[index] = NULL;
			}
			g_free(item);

			return;
		} else {
			previous_item = item;
			item = item->next;
		}
	}
}

void htab_clear(htab_t *table)
{
	htab_listitem *item = NULL;

	for(unsigned i = 0; i < table->size; i++) {
		while((item = table->list[i]) != NULL) {
			htab_remove(table, item->key);
		}
	}
}

void htab_free(htab_t *table)
{
	if(table != NULL) {
		htab_clear(table);
		g_free(table);
	}

	table = NULL;
}
