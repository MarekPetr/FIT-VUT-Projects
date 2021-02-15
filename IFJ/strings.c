/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "strings.h"
#include "errors.h"


int str_create(Tstring *str)
{
    str->data = ((char *) malloc(sizeof(char) * ALLOC_CHUNK));
    if (str->data == NULL) {
        fprintf(stderr,"Allocating space for str_create failed!\n");
        return ERR_INTERN;
    }

    str->length = 0;
    str->size = ALLOC_CHUNK;
    str->data[0] = '\0';
    return 0;
}

int str_create_spec(Tstring *str, int size)
{
    size++; // je potreba misto pro \0 
    str->length = 0;
    str->data = ((char *) malloc(sizeof(char) * size));
    if (str->data == NULL) {
        fprintf(stderr,"Allocating space for str_create_spec failed!\n");
        return ERR_INTERN;
    }

    str->size = size;
    str->data[0] = '\0';
    return 0;
}

void str_destroy(Tstring *str)
{
    str->length = 0;
    str->size = 0;
    free(str->data);
    str->data = NULL;
}

int str_clear(Tstring *str)
{
    str->data = ((char *) realloc(str->data, sizeof(char) * (ALLOC_CHUNK)));
    if (str->data == NULL) {
        free(str->data);
        fprintf(stderr,"Reallocating space for str_clear failed!\n");
        return ERR_INTERN;
    }

    str->length = 0;
    str->size = ALLOC_CHUNK;
    str->data[0] = '\0';
    return 0;
}


int str_push_char(Tstring *str, char c)
{
    if ( str->length+2 >= str->size) {
        str->data = ((char *) realloc(str->data, sizeof(char) * (str->length + ALLOC_CHUNK)));
        if (str->data == NULL) {
            free(str->data);
            fprintf(stderr,"Reallocating space for str_push_char failed!\n");
            return ERR_INTERN;
        }
        str->size = str->length + ALLOC_CHUNK; 
    }

    str->data[str->length] = c;
    str->data[++(str->length)] = '\0';
    return 0;
}

int str_pop_char(Tstring *str)
{   
    if (str->length <= 0)
        return 0;
    
    if (str->size > ALLOC_CHUNK) { // ALLOC_CHUNK bude vždy nejmenší alokovaná velikost
        int new_size = (((str->length / ALLOC_CHUNK) + 1) * ALLOC_CHUNK);
        str->data = ((char *) realloc(str->data, sizeof(char) * new_size));
            if (str->data == NULL) {
                free(str->data);
                fprintf(stderr,"Reallocating space for str_pop_char failed!\n");
                return ERR_INTERN;
            }
        str->size = new_size;
    }
    str->length--;
    str->data[str->length] = '\0'; // nahrazeni posledniho znaku '\0'
    return 0;
}

int str_append_str(Tstring *target, Tstring *to_append)
{
    int err = 0;
    for (int i = 0; i < to_append->length; i++)
    {
        if ((err = str_push_char(target, to_append->data[i])) != 0)
            return err;
    }
    return 0;
}

int str_create_init(Tstring *str, char *data)
{
    int length = strlen(data);
    str->data = ((char *) malloc(sizeof(char) * length+1));
    if (str->data == NULL) {
        fprintf(stderr,"Allocating space for str_create_init failed!\n");
        return ERR_INTERN;
    }

    strcpy(str->data, data);
    str->length = length;
    str->size = length + 1; // \0 pridan implicitne za data
    return 0;
}

int str_rewrite_data(Tstring *str, char *data)
{
    int length = strlen(data);
    
    if (length > str->length) {
        str->data = ((char *) realloc(str->data, sizeof(char) * length+1));
        if (str->data == NULL) {
            fprintf(stderr,"Allocating space for str_rewrite_data failed!\n");
            return ERR_INTERN;
        } 
        str->size = length + 1; // \0 pridan implicitne za data
     }
    str->length = length;
     
    strcpy(str->data, data);

    return 0;
}

int str_delete_index(Tstring *str, int index)
{
    if (index > str->length) {
        fprintf(stderr, "Trying to change nonexisting index in an array for str_delete_index!\n");
        return ERR_INTERN;
    }

    int err = 0;
    int to_move = str->length - index;

    for (int i = 0; i < to_move; i++)
    {
        str->data[i+index] = str->data[i+index+1];
    }
    
    // v pripade potreby odalokuje pamet tak, aby nebylo volne vice nez ALLOC_CHUNK
    if (str->size > ALLOC_CHUNK) { // ALLOC_CHUNK bude vždy nejmenší alokovaná velikost
        int new_size = (((str->length / ALLOC_CHUNK) + 1) * ALLOC_CHUNK);
        str->data = ((char *) realloc(str->data, sizeof(char) * new_size));
            if (str->data == NULL) {
                free(str->data);
                fprintf(stderr,"Reallocating space for str_delete_index failed!\n");
                return ERR_INTERN;
            }
        str->size = new_size;
    }
    str->length--;

    return err;
}

void delete_last_index(Tstring *str)
{
    int last_index = --(str->length);
    str->data[last_index] = '\0';
}