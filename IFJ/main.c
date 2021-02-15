/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#include <stdio.h>
#include <stdlib.h>
/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#include <stdbool.h>
#include "lex.h"
#include "strings.h"
#include "parser.h"
#include "symbol.h"
#include "symtable.h"
#include "errors.h"
#include "code_gen_expres.h"
#include "garbage.h"

int p = 0;
int ar_count = 0;
int if_counter = 0;
int while_counter = 0;
int whole_div_cntr = 0;
int equal = 1;
int arg_cnt = 0; 

Tstate last_gen_type; 

int main() {
    int error = 0;
    if ((error = str_create(&(token.t_str))) != 0) {
        return error;
    }

    token.t_line = 1;

    global_init();

    int parser;
    parser = rule_start_state();

    if (parser != 0){
    	error = parser;
      switch (error) {
        case 1:
          fprintf(stderr, "ERROR - 1 - chyba v programu v rámci lexikální analýzy (chybná struktura aktuálního lexému).\n");
          break;
        case 2:
          fprintf(stderr, "ERROR - 2 - chyba v programu v rámci syntaktické analýzy (chybná syntaxe programu).\n");
          break;
        case 3:
          fprintf(stderr, "ERROR - 3 - sémantická chyba v programu – nedefinovaná funkce/proměnná, pokus o redefinici funkce/proměnné, atd.\n");
          break;
        case 4:
          fprintf(stderr, "ERROR -  4 - sémantická chyba typové kompatibility v aritmetických, řetězcových a relačních výrazech, příp. špatný počet či typ parametrů u volání funkce.\n");
          break;
        case 6:
          fprintf(stderr, "ERROR -  6 - ostatní sémantické chyby.\n");
          break;
        case 99:
          fprintf(stderr, "ERROR -  99 - interní chyba překladače tj. neovlivněná vstupním programem (např. chyba alokace paměti, atd.).\n");
          break;
      }
    	printf("ERROR on line %d\n", token.t_line);

    }
    else {
      //printf("%s",gen_str.data);
      //printf("\n\n\nPARSER PROBEHL USPESNE\n");
    }

    str_destroy(&(token.t_str));

    global_free();

    return error;
}
