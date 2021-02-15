/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#ifndef LEX_H
#define LEX_H

#include "strings.h"

typedef enum {

/******NEMENIT ANI NEPRIDAVAT******/
/******ZDE ZALEZI NA PORADI********/
// KLICOVA SLOVA
/*0*/  st_as,
/*1*/  st_asc,
/*2*/  st_declare,
/*3*/  st_dim,
/*4*/  st_do,
/*5*/  st_double,
/*6*/  st_else,
/*7*/  st_end,
/*8*/  st_chr,
/*9*/  st_function,
/*10*/ st_if,
/*11*/ st_input,
/*12*/ st_integer,
/*13*/ st_length,
/*14*/ st_loop,
/*15*/ st_print,
/*16*/ st_return,
/*17*/ st_scope,
/*18*/ st_string,
/*19*/ st_substr,
/*20*/ st_then,
/*21*/ st_while,

// REZERVOVANA KLICOVA SLOVA
/*22*/ st_and,
/*23*/ st_boolean,
/*24*/ st_continue,
/*25*/ st_elseif,
/*26*/ st_exit,
/*27*/ st_false,
/*28*/ st_for,
/*29*/ st_next,
/*30*/ st_not,
/*31*/ st_or,
/*32*/ st_shared,
/*33*/ st_static,
/*34*/ st_true,
/******NEMENIT ANI NEPRIDAVAT******/

// DALSI SMERODATNE STAVY
/*35*/ st_id,             /*identifikator  */
/*36*/ st_del,            /* /   */
/*37*/ st_del_cele,       /* \   */
/*38*/ st_nas,            /* *   */
/*39*/ st_scit,           /* +   */
/*40*/ st_odcit,          /* -   */
/*41*/ st_mensi,          /* <   */
/*42*/ st_vetsi,          /* >   */
/*43*/ st_menrov,         /* <= */
/*44*/ st_vetrov,         /* >= */
/*45*/ st_rovno,          /* =  */
/*46*/ st_nerov,          /* <> */
/*47*/ st_stred,          /* ;  */
/*48*/ st_carka,          /* ,  */
/*49*/ st_tecka,          /* .  */
/*50*/ st_levzav,         /* (  */
/*51*/ st_pravzav,        /* )  */
/*52*/ st_retez,          /* retezec */
/*53*/ st_int_val,        /* hodnota int bez exp*/
/*54*/ st_double_val,     /* hodnota double bez exp*/
/*55*/ st_exp_int,        /* int s exp*/
/*56*/ st_exp_doub,       /* double s exp*/
/*57*/ st_eol,
/*58*/ st_eof,
/*59*/ st_error,

// POMOCNE STAVY
/*60*/ st_begin,
/*61*/ st_final,
/*62*/ st_vykric,         /* !  */
/*63*/ st_radek_kom,      /* '  */
/*64*/ st_blok_kom_0,     /* /' */
/*65*/ st_blok_kom_1,     /* /'koment */
/*66*/ st_blok_kom_2,     /* '/ */ 
/*67*/ st_retLZ,          /* retezcovy literal zacatek: !" */
/*68*/ st_esc,            /* escape znak \ */
/*69*/ st_esc_num,
/*70*/ st_double_val_dot,
/*71*/ st_exp_int_e,
/*72*/ st_exp_int_s,
/*73*/ st_exp_doub_e,
/*74*/ st_exp_doub_s,

} Tstate;

typedef struct t_token {
    Tstring t_str;         // jeden token ze vstupu (např."integer")
    Tstate t_state;        // int, double, string, as, asc,  ...
    int t_line;            // radek tokenu
} Ttoken;

Ttoken token;   // globalni token

int generate_token();

#endif