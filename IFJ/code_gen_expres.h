/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#ifndef GEN_EXPR_H
#define GEN_EXPR_H

#include "lex.h"

Tstate return_type(Tstring *operand);
int expr_gen(int operator, Tstring *operand_1, Tstring *operand_2, Tstate dest_type, Tstate instruct);  

#endif