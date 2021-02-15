/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#ifndef ERRORS_H
#define ERRORS_H

#define ERR_LEX 1

#define ERR_SYN 2

#define ERR_SEM_PROG 3

#define ERR_SEM_TYPE 4

#define ERR_SEM_OTHER 6

#define ERR_INTERN 99

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define debug_print(fmt, ...) \
        do { if (DEBUG_TEST) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

/* Pouzití: Nad #include "errors.h" #define DUBUG
   Výpisy:  debug_print("%s\n", "Popis chyby");*/

#endif