/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#include <stdlib.h>
#include <stdio.h>
#include "expresion.h"
#include "code_gen_expres.h"
#include "lex.h"
#include "symbol.h"
#include <string.h>
#include <ctype.h>

extern int p;
extern int whole_div_cntr;
extern Tstate last_gen_type;
extern int equal;
int relation_cntr;

Tstate return_type(Tstring *operand)
{  
    Tstate last_index;
    Tstate op_type;
    last_index = operand->length;
    last_index--; 

    if (operand->length == 0) {
        op_type = st_string; 
    } else if (operand->data[last_index] == '#') {
        op_type = st_integer; 
        delete_last_index(operand);                   
    } else if (operand->data[last_index] == '/') {                
        op_type = st_double;
        delete_last_index(operand);            
    } else {
       op_type = st_string; 
    }
    return op_type;
}
int set_context(Tstring *context, int operand_type, Tstate dest_type, Tstring *operand) {
    double converted = 0.0;
    if ((strcmp(operand->data,"GF@&pomInteger") == 0) || (strcmp(operand->data,"GF@&pomFloat")  == 0)  ||  
        (strcmp(operand->data,"GF@&pomBool") == 0)) 
    {
        str_rewrite_data(context, "");

    } else if (operand_type != st_string && (isalpha(operand->data[0]) || operand->data[0] == '_')) {
        if (p == 0) str_rewrite_data(context, "TF@");
        else        str_rewrite_data(context, "LF@");
    
    } else if (operand_type == st_integer){
        if (dest_type == st_integer || dest_type == 0) { // dest_type 0 znamena, ze se vyraz nebude nikam prirazovat
            str_rewrite_data(context, "int@");
        }
        else if (dest_type == st_double || dest_type == 0) {
            str_rewrite_data(context, "float@");
        } else {
            fprintf(stderr, "Wrong types of operands!\n");
            return ERR_SEM_TYPE;
        }

    } else if (operand_type == st_double) {
        if (dest_type == st_double || dest_type == 0) {
            str_rewrite_data(context, "float@");
        }
        else if (dest_type == st_integer || dest_type == 0) {
            Tstring res;
            str_create(&res);
            str_rewrite_data(context, "int@");
            int val = converted = nearbyint(strtod(operand->data,NULL));
            sprintf(res.data, "%d", val);
            str_rewrite_data(operand, res.data);
            str_destroy(&res);
        }
        else {
            fprintf(stderr, "Wrong types of operands!\n");
            return ERR_SEM_TYPE;
        }
    } else if (operand_type == st_string) {
        if (dest_type == st_string || dest_type == 0)
            str_rewrite_data(context, "string@");
        else {
            fprintf(stderr, "Wrong types of operands!\n");
            return ERR_SEM_TYPE;
        }
    // identifikator promenne 
    } 

    return 0;
}


int expr_gen(int operator, Tstring *operand_1, Tstring *operand_2, Tstate dest_type, Tstate instruct) {

    int error;
    

    if ((instruct == st_if    || instruct == st_loop) && 
        (operator != ex_equal && operator != ex_notEq  &&
         operator != ex_less  && operator != ex_lessEq &&
         operator != ex_great && operator != ex_greatEq ))
    {
        fprintf(stderr, "In if or loop statement must be relational operator only.\n");
        return ERR_SEM_TYPE;
    }



    Tstate op1_type, op2_type;
    

    // st_string || st_integer || st_double
    op1_type = return_type(operand_1);
    op2_type = return_type(operand_2);

    /*printf("op1_type: %d, operand_1:%s\n",op1_type, operand_1->data);
    printf("op2_type: %d, operand_2:%s\n",op2_type, operand_2->data);*/

    //printf("dest_type: %d\n",dest_type);
    char *destination;

    if (dest_type == st_integer) {
        destination = "GF@&pomInteger";
        last_gen_type = st_integer;
    } else if (dest_type == st_double) {
        destination = "GF@&pomFloat";
        last_gen_type = st_double;
    } else if (dest_type == st_string) {
        destination = "GF@&pomString";
        last_gen_type = st_string;
    } else if (dest_type == 0){ // POKUD NENI DESTINATION, PRIRAZUJE SE DO FLOAT, NELZE SE STRINGAMA
        if (op1_type == st_double && op2_type == st_double) {
            destination = "GF@&pomFloat";
            last_gen_type = st_double;
        } else if (op1_type == st_integer && op2_type == st_integer) {
            destination = "GF@&pomInteger";
            last_gen_type = st_integer;
        } else if (op1_type == st_double && op2_type == st_integer) {
            destination = "GF@&pomFloat";
            last_gen_type = st_double;
        } else if (op1_type == st_integer && op2_type == st_double) {
            destination = "GF@&pomFloat";
            last_gen_type = st_double;
        } else if (op1_type == st_string && op2_type == st_string) {
            destination = "GF@&pomString";
            last_gen_type = st_string;
        } else {
            fprintf(stderr, "Wrong types in expression (generator error).\n");
            return ERR_SEM_TYPE;
        }
        if (operator == ex_equal  || operator == ex_notEq || operator == ex_less || 
            operator == ex_lessEq || operator == ex_great || operator == ex_greatEq)
        {
            if (op1_type != op2_type) {
                fprintf(stderr, "Wrong types of operands in relational expression\n");
                return ERR_SEM_TYPE;
            }
        }
    }

    char* context = (p == 0 ? "TF@" : "LF@");
    Tstring context_1;
    Tstring context_2;
    str_create(&context_1);
    str_create(&context_2);

    error = set_context(&context_1, op1_type, dest_type, operand_1);  
    if (error == 0) {
        error = set_context(&context_2, op2_type, dest_type, operand_2); 
        if (error != 0){
            str_destroy(&context_1);
            str_destroy(&context_2);
            return error;
        }
    } 

    switch(operator) {
        
        case ex_mul:
        {
            printf("MUL %s %s%s %s%s\n", destination, context_1.data, operand_1->data, context_2.data, operand_2->data);            
            break;
        }

        case ex_div:
        {
            printf("DIV %s %s%s %s%s\n", destination, context_1.data, operand_1->data, context_2.data, operand_2->data);            
            break;
        }

        case ex_plus:
        {   
            if (op1_type == st_string) {
                printf("CONCAT GF@&pomString GF@&pomString %s%s\n", context_2.data, operand_2->data);                
            } else {
                printf("ADD %s %s%s %s%s\n", destination, context_1.data, operand_1->data, context_2.data, operand_2->data);              
            }
            break;
        }

        case ex_minus:
        {
            printf("SUB %s %s%s %s%s\n", destination, context_1.data, operand_1->data, context_2.data, operand_2->data);            
            break;
        }

        case ex_equal:
        {

            printf("EQ GF@&pomBool %s%s %s%s\n", context_1.data, operand_1->data, context_2.data, operand_2->data);
            equal = 1;         
            break;
        }

        case ex_notEq:
        {

            printf("EQ GF@&pomBool %s%s %s%s\n", context_1.data, operand_1->data, context_2.data, operand_2->data);
            equal = 0;            
            break;
        }

        case ex_lessEq:
        {
            printf("LT GF@&pomBool %s%s %s%s\n", context_1.data, operand_1->data, context_2.data, operand_2->data);
            printf("JUMPIFEQ $$comp_%d GF@&pomBool bool@true\n", relation_cntr);///
            printf("EQ GF@&pomBool %s%s %s%s\n", context_1.data, operand_1->data, context_2.data, operand_2->data);
            printf("LABEL $$comp_%d\n", relation_cntr++);
            equal = 1;                   
            break;
        }

        case ex_greatEq:
        {
            printf("GT GF@&pomBool %s%s %s%s\n", context_1.data, operand_1->data, context_2.data, operand_2->data);
            printf("JUMPIFEQ $$comp_%d GF@&pomBool bool@true\n", relation_cntr);///
            printf("EQ GF@&pomBool %s%s %s%s\n", context_1.data, operand_1->data, context_2.data, operand_2->data);
            printf("LABEL $$comp_%d\n", relation_cntr++);
            equal = 1;                      
            break;
        }


        case ex_less:
        {
            printf("LT GF@&pomBool %s%s %s%s\n", context_1.data, operand_1->data, context_2.data, operand_2->data);
            equal = 1;            
            break;
        }

        case ex_great:
        {
            printf("GT GF@&pomBool %s%s %s%s\n", context_1.data, operand_1->data, context_2.data, operand_2->data);
            equal = 1;            
            break;
        }

        case ex_wholeDiv:        
        {   
            printf("\nMOVE GF@&pomCntr int@0\n");
            printf("LABEL $$WholeDiv_Begin%d\n", whole_div_cntr);
            printf("ADD GF@&pomCntr GF@&pomCntr int@1\n");
            printf("SUB GF@&pomInteger GF@&pomInteger %s%s\n", context_2.data, operand_2->data); 
            printf("LT GF@&pomBool GF@&pomInteger int@0\n");
            printf("JUMPIFNEQ $$WholeDiv_Begin%d GF@&pomBool bool@true\n", whole_div_cntr++);/// counter
            printf("SUB GF@&pomCntr GF@&pomCntr int@1\n");
            printf("MOVE GF@&pomInteger GF@&pomCntr\n");
            break;
        }

        default:
        {
            fprintf(stderr, "Chyba v code_gen_expres %s%s %s%s %s%s\n", context, destination, context_1.data, operand_1->data, context_2.data, operand_2->data);
            str_destroy(&context_1);
            str_destroy(&context_2);
            return ERR_SEM_TYPE;
        }

    }

    str_destroy(&context_1);
    str_destroy(&context_2);
    return 0;
}