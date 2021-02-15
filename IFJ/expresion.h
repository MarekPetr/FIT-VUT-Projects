/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#ifndef EXPRESION
#define EXPRESION

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lex.h"
#include "string.h"
#include "errors.h"

#define err_malloc -1
#define err_StackEmpty -2
#define err_nullPtr -3


//Definice datovych typu pr zasobnik
//#-------------------------------------------#//

typedef struct tselem {
	int type;
	Tstring string;
	struct tselem *prevPtr;
	struct tselem *nextPtr;
} TSElem;

typedef struct {
	TSElem *topPtr;
	TSElem *activePtr;
} TStack;


//funkce definujici zasobnik
//#-------------------------------------------#//

int isTerminal (int symbol);
void SInit (TStack *s);
bool SEmpty (TStack *s);
int SPush (TStack *s, int tokenType , char* string);
int SPostActiveInsert (TStack *s, int tokenType);
void SPop (TStack *s);
int STopType (TStack *s);
char* STopString (TStack *s);
int SActive(TStack *s);
void SClean (TStack *s);

//vycty stavu precedencni analyzi
//#-------------------------------------------#//

int precedent_analysis(Tstate instruct, Tstate dest_type);
int set_operator();
int expresion_reduction(TStack *s, Tstate instruct, int reduce_counter, Tstring *ret_string, Tstate dest_type);

typedef enum {
	LT, // <
	GT, // >
	EQ, // ==
	XX, // chyba
}PrecTabValues;

enum {
/*1*/	ex_mul,
/*2*/	ex_div,      /* / */
/*3*/	ex_wholeDiv, /* \ */
/*4*/	ex_plus,
/*5*/	ex_minus,
/*6*/	ex_equal, 	 /* =  */
/*7*/	ex_notEq, 	 /* <> */
/*8*/	ex_less,
/*9*/	ex_lessEq,
/*10*/	ex_great,
/*11*/	ex_greatEq,
/*12*/	ex_leftBrac, /* ( */
/*13*/	ex_rightBrac,
/*14*/	ex_ident,	 /* i */
/*15*/	ex_integer,
/*16*/	ex_double,
/*17*/	ex_str,
/*19*/	ex_dollar, // zacatek zpracovani vyrazu
/*20*/	ex_reduction,
/*21*/	ex_rule_begin, //prednostni symbol
} EXSymbols;

#endif
