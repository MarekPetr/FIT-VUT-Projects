/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#ifndef PARSER
#define PARSER

#include "lex.h"
#include "symbol.h"
#include "symtable.h"
#include "errors.h"
#include "expresion.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HTAB_SIZE 23

Tstring gen_str;

//Funkce rekurzivniho sestupu
int skip_blank_lines();
int rule_start_state();
int rule_scope();
int rule_function();
int rule_function_dec(); //
int rule_function_head(); //
int rule_function_tail();
int rule_par(function_data *data_f); //
int rule_next_par(function_data *data_f); //
int rule_check_par(); //
int rule_check_next_par(); //
int rule_type(variable_data *data); //
int rule_ret_type(function_data *data_f); //
int rule_check_ret_type(); //
int rule_arg_type(function_data *data_f); //
int rule_check_arg_type(); //
int rule_st_list();
int rule_stat();
int rule_eval(Tstring id);
int rule_assign(Tstring id);
int rule_call_par();
int rule_call_next_par();
int rule_pr_expr();

#endif
