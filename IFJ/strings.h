/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#ifndef STRINGS_H
#define STRINGS_H

#define ALLOC_CHUNK 10

typedef struct tstring{
    int length; // aktualni delka 
    int size;   // alokovano
    char *data;
} Tstring;

extern Tstring gen_str;

// vytvori string (naalokuje ALLOC_CHUNK bytu, pote pridava)      
int str_create(Tstring *str);  

// vytvori string a alokuje mu velikost size (spec jako specific)
int str_create_spec(Tstring *str, int size); 

// uvolni string z pameti. Pote neni potreba nic dalsiho delat.
void str_destroy(Tstring *str);

// nastavi hodnoty na pocatecni, misto je stale alokovano stejne
int str_clear(Tstring *str);

// appenduje char do retezce
int str_push_char(Tstring *str, char c);

// odstrani posledni znak z retezce
int str_pop_char(Tstring *str);

// appenduje retezec do retezce
int str_append_str(Tstring *target, Tstring *to_append);

// alokuje a inicializuje string tak, aby hned obsahoval data
int str_create_init(Tstring *str, char *data);

// smaze znak na pozici 'index' a posune retezec tak, aby zaplnil volne misto
// v pripade, ze index je vyssi nez delka pole neudela nic
int str_delete_index(Tstring *str, int index);

// prepise data ve stringu na 'data'.
int str_rewrite_data(Tstring *str, char *data);

void delete_last_index(Tstring *str);
#endif