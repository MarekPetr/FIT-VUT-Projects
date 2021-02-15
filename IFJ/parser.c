/*
 Implementace prekladace imperativniho jazyka IFJ17
 Petr Marek,       login: xmarek66
 Jakub Stefanisin, login: xstefa22
 Petr Knetl,       login: xknetl00
*/

#include "parser.h"
#include "expresion.h"
#include "code_gen_expres.h"
#include "lex.h"
#include "errors.h"
#include "garbage.h"

extern int p;
extern int ar_count;
extern int if_counter;
extern int while_counter;
extern int equal;
extern Tstate last_gen_type;
extern int arg_cnt;
Tstring param;
int index_par = 1;

/**************************FUNKCE-REKURZIVNIHO-SESTUPU*********************************/
/**************************************************************************************/

void init_gen() {
        printf("LABEL $$scope\n");
        printf("DEFVAR GF@&pomInteger\n");///
        printf("MOVE GF@&pomInteger int@0\n");
        printf("DEFVAR GF@&pomString\n");///
        printf("MOVE GF@&pomString string@\n");
        printf("DEFVAR GF@&pomFloat\n");///
        printf("MOVE GF@&pomFloat float@0\n");
        printf("DEFVAR GF@&pomBool\n");///
        printf("MOVE GF@&pomBool bool@false\n");///
        printf("DEFVAR GF@&pomCntr\n");///
        printf("MOVE GF@&pomCntr int@0\n");///
        printf("CREATEFRAME\n\n");///
}

int skip_blank_lines(){
    int func_return;
    while(token.t_state == st_eol){
        if((func_return = generate_token())) return func_return;
    }
    return 0;
}

int rule_start_state(){ // stav <start t_state>
    int return_value = ERR_SYN;
    int func_return;
    if((func_return = generate_token())) return func_return;
    if((func_return = skip_blank_lines())) return func_return;
    if(token.t_state == st_scope || token.t_state == st_declare || token.t_state == st_function){ // simulace pravidla 1.
        printf(".IFJcode17\n");///
        printf("JUMP $$scope\n\n");
        if ((return_value = rule_function()) == 0)
            return_value = rule_scope();
    }
    return return_value;
}

int rule_scope() { // pravidlo <scope>
    int return_value = ERR_SYN;
    int func_return;
    if((func_return = skip_blank_lines())) return func_return;
    if(token.t_state == st_scope){ // simulace pravidla 2.
        init_gen();
        if((func_return = generate_token())) return func_return;
        if((return_value = rule_st_list()) == 0){
            if((func_return = skip_blank_lines())) return func_return;
            if(token.t_state == st_end){
                if((func_return = generate_token())) return func_return;

                if(token.t_state == st_scope){
                    if((func_return = generate_token())) return func_return;
                    if((func_return = skip_blank_lines())) return func_return;

                    if(token.t_state != st_eof) return ERR_SYN;

                    return_value = 0;
                }
            }

        }
    }
    return return_value;
}

int rule_function() { // stav <function>
    int return_value = ERR_SYN;
    int func_return;
    if((func_return = skip_blank_lines())) return func_return;
    if(token.t_state == st_function){ // simulace pravidla 3.
            if((return_value = rule_function_head()) == 0)
                if((return_value = rule_st_list()) == 0)
                    if((return_value = rule_function_tail()) == 0)
                        if((return_value = rule_function()) == 0)
                            return_value = 0;
    }
    else if(token.t_state == st_declare){ //simulace pravidla 4.
            if((return_value = rule_function_dec()) == 0)
                if((return_value = rule_function()) == 0)
                    return_value = 0;
    }
    else if(token.t_state == st_scope){ //simulace pravidla 5.
        return_value = 0;
    }

    return return_value;

}

int rule_function_dec() { // stav <function-dec>
    int return_value = ERR_SYN;
    int func_return;

    if(token.t_state == st_declare){ // simulace pravidla 6.
        if((func_return = generate_token())) return func_return;

        if(token.t_state == st_function){
            if((func_return = generate_token())) return func_return;

            if(token.t_state == st_id){
                htab_listitem *item;
                item = htab_find(global.global_table, token.t_str.data);
                if(item != NULL) {
                    return ERR_SEM_PROG;
                }
                item = htab_lookup_add(global.global_table, token.t_str.data);
                function_init(item, token.t_str.data);

                htab_t *local_table = htab_init(HTAB_SIZE);
                if(local_table == NULL) {
                    return ERR_INTERN;
                }

                item->pointer.function->local_symbol_table = local_table;
                if((func_return = generate_token())) return func_return;

                if(token.t_state == st_levzav){
                    if((func_return = generate_token())) return func_return;

                    if((return_value = rule_par(item->pointer.function)) == 0){

                        if(token.t_state == st_pravzav){
                            if((func_return = generate_token())) return func_return;

                            if(token.t_state == st_as){
                                if((func_return = generate_token())) return func_return;

                                if((return_value = rule_ret_type(item->pointer.function)) == 0){
                                    return_value = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return return_value;
}

int rule_function_head() { // stav <function-head>
    int return_value = ERR_SYN;
    int func_return;
    Tstring ident;///
    p = 1;

    if(token.t_state ==  st_function) { // simulace pravidla 7.
        if((func_return = generate_token())) return func_return;

        if(token.t_state == st_id) {
            if(global.current_func_name == NULL) {
                global.current_func_name = g_malloc(sizeof(char *) * token.t_str.length);
            } else {
                global.current_func_name = g_realloc(global.current_func_name, sizeof(char *) * token.t_str.length);
            }
            strcpy(global.current_func_name, token.t_str.data);
            //
            htab_listitem *item;
            item = htab_find(global.global_table, token.t_str.data);
            if((item != NULL && item->pointer.function->defined == 1)) {
                return ERR_SEM_PROG;
            }

            if(item == NULL) {
                item = htab_lookup_add(global.global_table, token.t_str.data);
                function_init(item, token.t_str.data);

                str_create_init(&ident, token.t_str.data);

                htab_t *local_table = htab_init(HTAB_SIZE);
                if(local_table == NULL) {
                    return ERR_INTERN;
                }

                item->pointer.function->local_symbol_table = local_table;
                if((func_return = generate_token())) return func_return;

                if(token.t_state == st_levzav) {
                    if((func_return = generate_token())) return func_return;

                    if((return_value = rule_par(item->pointer.function)) == 0) {

                        if(token.t_state == st_pravzav) {
                            if((func_return = generate_token())) return func_return;

                            if(token.t_state == st_as) {
                                if((func_return = generate_token())) return func_return;

                                if((return_value = rule_ret_type(item->pointer.function)) == 0) {
                                    item->pointer.function->defined = 1;

                                    global.local_sym = item->pointer.function->local_symbol_table;
                                    global.current_arguments = item->pointer.function->arguments;
                                    if(token.t_state == st_eol) {
                                        // TODO FUNKCE
                                        printf("LABEL $%s\n", ident.data);///
                                        printf("PUSHFRAME\n");
                                        printf("DEFVAR LF@&retval\n");///
                                        str_destroy(&ident);///
                                        if((func_return = generate_token())) return func_return;
                                        return_value = 0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                item->pointer.function->defined = 1;

                global.local_sym = item->pointer.function->local_symbol_table;
                global.current_arguments = item->pointer.function->arguments;


                str_create_init(&ident, token.t_str.data);///
                if((func_return = generate_token())) return func_return;

                if(token.t_state == st_levzav) {
                    if((func_return = generate_token())) return func_return;
                    printf("LABEL $%s\n", ident.data);///
                    printf("PUSHFRAME\n");
                    printf("DEFVAR LF@&retval\n");///
                    str_destroy(&ident);///
                    if((return_value = rule_check_par()) == 0) {

                        if(token.t_state == st_pravzav) {
                            if(check_argument_count(ar_count) != 1) {
                                debug_print("%s\n", "Argument count does not match with declaration");
                                return ERR_SEM_PROG;
                            }
                            if((func_return = generate_token())) return func_return;

                            if(token.t_state == st_as) {
                                if((func_return = generate_token())) return func_return;

                                if((return_value = rule_check_ret_type()) == 0) {

                                    if(token.t_state == st_eol) {                                        
                                        if((func_return = generate_token())) return func_return;
                                        return_value = 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return return_value;
}

int rule_function_tail(){ // stav <function-tail>
    int return_value = ERR_SYN;
    int func_return;
    if((func_return = skip_blank_lines())) return func_return;
    if(token.t_state == st_end){ // simulace pravidla 8.
        Tstate type = return_function_type();
        if (type == st_integer) printf("MOVE LF@&retval GF@&pomInteger\n");
        if (type == st_string)  printf("MOVE LF@&retval GF@&pomString\n");
        if (type == st_double)  printf("MOVE LF@&retval GF@&pomFloat\n");
        arg_cnt = 0;
        printf("POPFRAME\n");
        printf("RETURN\n\n");
        if((func_return = generate_token())) return func_return;

        if(token.t_state == st_function){
            if((func_return = generate_token())) return func_return;
            return_value = 0;
        }
    }
    p = 0;
    ar_count = 0;
    return return_value;
}

int rule_par(function_data *data_f){ // stav <par>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state == st_id){ // simulace pravidla 9.
        htab_listitem *item;
        item = htab_find(global.global_table, token.t_str.data);
        if(item != NULL && item->type == type_function) {
            return ERR_SEM_PROG;
        }

        if((item = htab_find(data_f->local_symbol_table, token.t_str.data)) != NULL) {
            printf("Function has another parameter with same name");
            return ERR_SEM_PROG;
        }
        item = htab_lookup_add(data_f->local_symbol_table, token.t_str.data);
        variable_init(item, token.t_str.data);

        if(add_argument_function(data_f, &token) != 0) {
            return ERR_INTERN;            
        }
        if((func_return = generate_token())) return func_return;

        if(token.t_state == st_as){
            if((func_return = generate_token())) return func_return;

            if((return_value = rule_arg_type(data_f)) == 0)
                if((return_value = rule_type(item->pointer.variable)) == 0)
                    if((return_value = rule_next_par(data_f)) == 0){
                        return_value = 0;
            }
        }
    }

    else if(token.t_state == st_pravzav){ //simulace pravidla 10.
        return_value = 0;
    }

    return return_value;
}

int rule_next_par(function_data *data_f){ // stav <next-par>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state ==  st_carka){ // simulace pravidla 11.

        if((func_return = generate_token())) return func_return;

        if((return_value = rule_par(data_f)) == 0){
            return_value = 0;
        }
    }
    else if(token.t_state == st_pravzav){ // simulace pravidla 12.
        return_value = 0;
    }


    return return_value;


}

int rule_check_par(){ // stav <check-par>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state == st_id){ // simulace pravidla 9.
        ar_count++;
        if(check_argument_name(token.t_str.data, ar_count) != 1) {
            htab_remove(global.local_sym, global.current_arguments[ar_count - 1].argument_name.data);
            htab_listitem *item;
            item = htab_lookup_add(global.local_sym, token.t_str.data);
            variable_init(item, token.t_str.data);
            item->pointer.variable->type = global.current_arguments[ar_count - 1].type;

            str_clear(&global.current_arguments[ar_count - 1].argument_name);
            str_append_str(&(global.current_arguments[ar_count - 1].argument_name), &token.t_str);           
        }
        char* context = (p == 0 ? "TF@" : "LF@");
        printf("DEFVAR %s%s\n",context, global.current_arguments[ar_count - 1].argument_name.data);
        printf("MOVE %s%s %s&arg%d\n", context, global.current_arguments[ar_count - 1].argument_name.data, context, arg_cnt++);
        if((func_return = generate_token())) return func_return;

        if(token.t_state == st_as){
            if((func_return = generate_token())) return func_return;

            if((return_value = rule_check_arg_type()) == 0) {
                if((func_return = generate_token())) return func_return;

                if((return_value = rule_check_next_par()) == 0){
                    return_value = 0;
                }
            }
        }
    }

    else if(token.t_state == st_pravzav){ //simulace pravidla 10.
        return_value = 0;
    }
    return return_value;
}

int rule_check_next_par(){ // stav <check-next-par>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state ==  st_carka){ // simulace pravidla 11.

      if((func_return = generate_token())) return func_return;

        if((return_value = rule_check_par()) == 0){
            return_value = 0;
        }
    }
    else if(token.t_state == st_pravzav){ // simulace pravidla 12.
        return_value = 0;
    }

    return return_value;
}

int rule_type(variable_data *data){ // stav <type>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state == st_integer || token.t_state == st_double || token.t_state == st_string){ // simulace pravidla 13., 14., 15.
        set_type_variable(data, &token);
        if((func_return = generate_token())) return func_return;
        return_value = 0;
    }

    return return_value;
}

int rule_ret_type(function_data *data_f){ // stav <ret-type>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state == st_integer || token.t_state == st_double || token.t_state == st_string){ // simulace pravidla 13., 14., 15.
        set_return_type_function(data_f, &token);
        if((func_return = generate_token())) return func_return;
        return_value = 0;
    }

    return return_value;
}

int rule_check_ret_type(){ // stav <check-ret-type>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state == st_integer || token.t_state == st_double || token.t_state == st_string){ // simulace pravidla 13., 14., 15.
        if(check_function_return_type(token.t_state) == 0) {
            return ERR_SEM_PROG;
        }
        if((func_return = generate_token())) return func_return;
        return_value = 0;
    }

    return return_value;
}

int rule_arg_type(function_data *data_f){ // stav <arg-type>
    int return_value = ERR_SYN;
    if(token.t_state == st_integer || token.t_state == st_double || token.t_state == st_string){ // simulace pravidla 13., 14., 15.
        set_argument_type_function(data_f, &token);
        return_value = 0;
    }

    return return_value;
}

int rule_check_arg_type(){ // stav <check-arg-type>
    int return_value = ERR_SYN;
    if(token.t_state == st_integer || token.t_state == st_double || token.t_state == st_string){ // simulace pravidla 13., 14., 15.
        if(check_argument_type(token.t_state, ar_count) != 1) {
            return ERR_SEM_PROG; //Neodpovedajuci datovy typ argumentu
        }
        return_value = 0;
    }

    return return_value;
}

int rule_st_list(){ // stav <st-list>
    int return_value = ERR_SYN;
    int func_return;
    if((func_return = skip_blank_lines())) return func_return;
    if(token.t_state == st_id || token.t_state == st_dim || token.t_state == st_input ||
       token.t_state == st_print || token.t_state == st_if || token.t_state == st_do ||
       token.t_state == st_return  ){ // simulace pravidla 16.
                if((return_value = rule_stat()) == 0)
                    if((return_value = rule_st_list()) == 0)
                        return_value = 0;
    }

    else if(token.t_state == st_end || token.t_state == st_else|| token.t_state == st_loop){ // simulace pravidla 17.
        return_value = 0;
    }

    return return_value;
}

int rule_stat(){ // stav <stat>
    int return_value = ERR_SYN;
    int func_return;
    Tstring ident;///
    Tstate instruct = 0;
    Tstate dest_type = 0;

    if(token.t_state == st_dim ){ // simulace pravidla 18.

        if((func_return = generate_token())) return func_return;

        if(token.t_state == st_id){
            htab_listitem *item;
            item = htab_find((p == 1 ? global.local_sym : global.global_table), token.t_str.data);
            if(item != NULL) {
                fprintf(stderr, "Variable already defined\n");
                return ERR_SEM_PROG; //Premmenna nebola vramci danej funkcie deklarovana
            }

            item = htab_lookup_add((p == 1 ? global.local_sym : global.global_table), token.t_str.data);
            variable_init(item, token.t_str.data);
            str_create_init(&ident, token.t_str.data);///
            if((func_return = generate_token())){
                str_destroy(&ident);///
                return func_return;
            }

            if(token.t_state == st_as){
                if((func_return = generate_token())){
                    str_destroy(&ident);///
                    return func_return;
                }

                if((return_value = rule_type(item->pointer.variable)) == 0){
                    char* context = (p == 0 ? "TF@" : "LF@");
                    Tstate var_type = return_variable_type(ident.data);

                    printf("DEFVAR %s%s\n", context, ident.data);///
                    if      (var_type == st_integer) printf("MOVE %s%s int@0\n", context, ident.data);
                    else if (var_type == st_double)  printf("MOVE %s%s float@0\n", context, ident.data);
                    else if (var_type == st_string)  printf("MOVE %s%s string@\n", context, ident.data);

                    if((return_value = rule_eval(ident)) == 0){
                        return_value = 0;
                    }
                }
            }
            str_destroy(&ident);///
        }
    }

    else if(token.t_state == st_id){ // simulace pravidla 21.
        if(variable_exist(token.t_str.data) ==  0){
            fprintf(stderr,"Not defined variable\n");
            return ERR_SEM_PROG; //Premmenna nebola vramci danej funkcie deklarovana
        }
        str_create_init(&ident, token.t_str.data);
        if((func_return = generate_token())){
            str_destroy(&ident);///
            return func_return;
        }

        if(token.t_state == st_rovno){

            if((func_return = generate_token())){
                str_destroy(&ident);///
                return func_return;
            }
            if((return_value = rule_assign(ident)) == 0){
                return_value = 0;
            }
        }
        str_destroy(&ident);///
    }


    else if(token.t_state == st_input){ // simulace pravidla 28.
        if((func_return = generate_token())) return func_return;

        if(token.t_state == st_id){
            if(variable_exist(token.t_str.data) == 0) {
                debug_print("%s\n", "Not defined variable\n");
                return ERR_SEM_PROG; //Premmenna nebola vramci danej funkcie deklarovana
            }
            char* context = (p == 0 ? "TF@" : "LF@");
            Tstate type = return_variable_type(token.t_str.data);
            printf("WRITE string@?\\032\n");
            if      (type == st_integer) printf("READ %s%s int\n",context, token.t_str.data);///
            else if (type == st_double)  printf("READ %s%s float\n",context, token.t_str.data);///
            else if (type == st_string)  printf("READ %s%s string\n",context, token.t_str.data);///

            if((func_return = generate_token())) return func_return;

            return_value = 0;
        }
    }

    else if(token.t_state == st_print){ // simulace pravidla 29.
        instruct = st_print;
        if((func_return = generate_token())) return func_return;

        if((func_return = precedent_analysis(instruct, dest_type)) == 0){
            if(token.t_state == st_stred){
                if (last_gen_type == st_integer) {
                    printf("WRITE GF@&pomInteger\n");
                } else if (last_gen_type == st_double) {
                    printf("WRITE GF@&pomFloat\n");
                } else if (last_gen_type == st_string) {
                    printf("WRITE GF@&pomString\n");
                }
                if((func_return = generate_token())) return func_return;

                if((return_value = rule_pr_expr()) == 0){
                    return_value = 0;
                }
            }
        }
        else return func_return;
    }

    else if(token.t_state == st_if){ // simulace pravidla 32.
        instruct = st_if;
        if((func_return = generate_token())) return func_return;

        if((func_return = precedent_analysis(instruct, dest_type)) == 0){
            if_counter++;
            if (equal) {
                printf("JUMPIFNEQ $$else_%d GF@&pomBool bool@true\n", if_counter);///
            } else { 
                printf("JUMPIFEQ $$else_%d GF@&pomBool bool@true\n", if_counter);///
            }

            if(token.t_state == st_then){
                if((func_return = generate_token())) return func_return;

                if(token.t_state == st_eol){
                    if((func_return = generate_token())) return func_return;

                    if((return_value = rule_st_list()) == 0){
                        printf("JUMP $$endif_%d\n", if_counter);
                        printf("LABEL $$else_%d\n", if_counter);

                        if(token.t_state == st_else){
                            if((func_return = generate_token())) return func_return;

                            if(token.t_state == st_eol){
                                if((func_return = generate_token())) return func_return;

                                if((return_value = rule_st_list()) == 0){

                                    if(token.t_state == st_end){
                                        if((func_return = generate_token())) return func_return;

                                        if(token.t_state == st_if){
                                            printf("LABEL $$endif_%d\n", if_counter);
                                            if_counter--;
                                            if((func_return = generate_token())) return func_return;

                                            return_value = 0;
                                        }
                                    }
                                }
                            }
                        }
              else if(token.t_state == st_end){
                if((func_return = generate_token())) return func_return;
                  if(token.t_state == st_if){
                    if((func_return = generate_token())) return func_return;
                      return_value = 0;
                  }
              }
                    }
                }
            }
        }
        else return func_return;
    }//konec pravidla 32.

    else if(token.t_state == st_do){ // simulace pravidla 33.
        if((func_return = generate_token())) return func_return;

        if(token.t_state == st_while){
            instruct = st_loop;
            while_counter++;
            printf("LABEL $$loop_%d\n", while_counter);
            if((func_return = generate_token())) return func_return;

            if((func_return = precedent_analysis(instruct, dest_type)) == 0){

                if(token.t_state == st_eol){
                    if (equal)  {
                        printf("JUMPIFNEQ $$loop_end_%d GF@&pomBool bool@true\n", while_counter);
                    } else  {
                        printf("JUMPIFEQ $$loop_end_%d GF@&pomBool bool@true\n", while_counter);
                    }
                    if((func_return = generate_token())) return func_return;

                    if((return_value = rule_st_list()) == 0){
                        printf("JUMP $$loop_%d\n", while_counter);
                        if(token.t_state == st_loop){
                            printf("LABEL $$loop_end_%d\n", while_counter);
                            while_counter--;
                            if((func_return = generate_token())) return func_return;

                            return_value = 0;
                        }
                    }
                }

            }
            else return func_return;
        }
    }//konec 33. pravidla

    else if(token.t_state == st_return){ // simulace pravidla 34.

        if(p == 0) return ERR_SYN;  //st_return se nesmi vyskytovat uvnitr scopu

        if((func_return = generate_token())) return func_return;

        if((func_return = precedent_analysis(instruct, dest_type)) == 0){

            return_value = 0;
        }
        else return func_return;
    }

    return return_value;
}// konec funkce rule_stat()

int rule_eval(Tstring id){ // stav <eval>
    int return_value = ERR_SYN;
    int func_return;
    if((func_return = skip_blank_lines())) return func_return;
    Tstate instruct = 0;
    Tstate dest_type = 0;

    dest_type = return_variable_type(id.data);    
    char* context = (p == 0 ? "TF@" : "LF@");

    if(token.t_state == st_rovno ){ // simulace pravidla 19.
        if((func_return = generate_token())) return func_return;

        if((func_return = precedent_analysis(instruct, dest_type)) == 0){            
            if (dest_type == st_integer) {
                printf("MOVE %s%s GF@&pomInteger\n",context, id.data);
            } else if (dest_type == st_double) {
                printf("MOVE %s%s GF@&pomFloat\n",context, id.data);
            } else if (dest_type == st_string) {

                printf("MOVE %s%s GF@&pomString\n",context, id.data);

            }
            return_value = 0;
        }
        else return func_return;
    }

    else if(token.t_state == st_id || token.t_state == st_dim || token.t_state == st_input ||
         token.t_state == st_print || token.t_state == st_if  || token.t_state == st_do ||
        token.t_state == st_return || token.t_state == st_end ){ // simulace pravidla 20.

        return_value = 0;
    }
    return return_value;
}

int rule_assign(Tstring id){ // stav <assign>
    int return_value = ERR_SYN;
    int func_return;
    Tstate instruct = 0;
    Tstate dest_type = 0;
    char* context = (p == 0 ? "TF@" : "LF@");
    dest_type = return_variable_type(id.data);
    Tstring fce;

    if(token.t_state == st_id){ // simulace pravidla 23.

        htab_listitem *item;
        item = htab_find((p == 1 ? global.local_sym : global.global_table), token.t_str.data);
        if(item == NULL) { // tady konci vetsina vstupu TODO: OPRAVIT!
            item = htab_find(global.global_table, token.t_str.data);
            if(item == NULL || item->type != type_function) {
                fprintf(stderr, "Variable is not declared.\n");
                return ERR_SEM_PROG;
            }
        }
        str_create_init(&(fce),token.t_str.data);

        if(item->type != type_function){// identifikator neni funkce
          if((func_return = precedent_analysis(instruct, dest_type)) == 0){
            return_value = 0;
            if (dest_type == st_integer) {
                printf("MOVE %s%s GF@&pomInteger\n",context, id.data);
            } else if (dest_type == st_double) {
                printf("MOVE %s%s GF@&pomFloat\n",context, id.data);
            } else if (dest_type == st_string) {
                printf("MOVE %s%s GF@&pomString\n",context, id.data);
            }
          } else {
            str_destroy(&(fce));
            return func_return;
            }
          str_destroy(&(fce));
          return return_value;
        }
        if(item->pointer.function->defined == 0) {//identifikator nedefinovane funkce
            //free(item);
            return ERR_SEM_PROG;
        }

        global.local_sym = item->pointer.function->local_symbol_table;
        global.current_arguments = item->pointer.function->arguments;

        if((func_return = generate_token()))  {
            str_destroy(&(fce));
            return func_return;
        }
        if(token.t_state == st_levzav){
            if((func_return = generate_token()))  {
                str_destroy(&(fce));
                return func_return;
            }

            if((return_value = rule_call_par()) == 0){

                if(token.t_state == st_pravzav){
                    if((func_return = generate_token()))  {
                        str_destroy(&(fce));
                        return func_return;
                    }
                    index_par = 1;
                    // TODO FUNKCE
                    //debug_print("%s\n","funkce");
                    printf("CALL $%s\n",fce.data);
                    printf("MOVE %s%s %s&retval\n",context, id.data, context);

                    return_value = 0;
                }
            }
        }
    }// konec pravidla 23.
    else if(token.t_state == st_length) {
        if((func_return = generate_token())) return func_return;
        if(token.t_state == st_levzav) {
            if((func_return = generate_token())) return func_return;
            if(token.t_state == st_id) {
                Tstring par;
                str_create_init(&par, token.t_str.data);
                if((func_return = generate_token())) return func_return;
                if(token.t_state == st_pravzav) {
                    if((func_return = generate_token())) return func_return;
                    printf("STRLEN %s%s %s%s\n", context, id.data, context, par.data);

                    str_destroy(&par);
                    return_value = 0;
                }
            } else if(token.t_state == st_retez) {
                printf("MOVE GF@&pomString string@%s\n", token.t_str.data);
                if((func_return = generate_token())) return func_return;
                if(token.t_state == st_pravzav) {
                    if((func_return = generate_token())) return func_return;
                    printf("STRLEN %s%s GF@&pomString\n", context, id.data);

                    return_value = 0;
                }
            }
        }
    }
    else if(token.t_state == st_chr) {
        if((func_return = generate_token())) return func_return;
        if(token.t_state == st_levzav) {
            if((func_return = generate_token())) return func_return;
            if(token.t_state == st_id) {
                Tstring par;
                str_create_init(&par, token.t_str.data);
                if((func_return = generate_token())) return func_return;
                if(token.t_state == st_pravzav) {
                    if((func_return = generate_token())) return func_return;
                    printf("INT2CHAR %s%s %s%s\n", context, id.data, context, par.data);

                    str_destroy(&par);
                    return_value = 0;
                }
            } else if(token.t_state == st_int_val) {
                printf("MOVE GF@&pomInteger int@%s\n", token.t_str.data);
                if((func_return = generate_token())) return func_return;
                if(token.t_state == st_pravzav) {
                    if((func_return = generate_token())) return func_return;
                    printf("INT2CHAR %s%s GF@&pomInteger\n", context, id.data);

                    return_value = 0;
                } 
            }
        }
    }
    else if(token.t_state == st_asc) {
        if((func_return = generate_token())) return func_return;
        if(token.t_state == st_levzav) {
            if((func_return = generate_token())) return func_return;
            if(token.t_state == st_id) {
                Tstring par;
                str_create_init(&par, token.t_str.data);
                if((func_return = generate_token())) return func_return;
                if(token.t_state == st_carka) {
                    if((func_return = generate_token())) return func_return;
                    if(token.t_state == st_id) {
                        Tstring par2;
                        str_create_init(&par2, token.t_str.data);
                        if((func_return = generate_token())) return func_return;
                        if(token.t_state == st_pravzav) {
                            if((func_return = generate_token())) return func_return;
                            printf("STRI2INT %s%s %s%s %s%s\n", context, id.data, context, par.data, context, par2.data);

                            str_destroy(&par);
                            str_destroy(&par2);
                            return_value = 0;
                        }
                    } else if(token.t_state == st_int_val) {
                        printf("MOVE GF@&pomInteger int@%s\n", token.t_str.data);
                        if((func_return = generate_token())) return func_return;
                        if(token.t_state == st_pravzav) {
                            if((func_return = generate_token())) return func_return;
                            printf("STRI2INT %s%s %s%s GF@&pomInteger\n", context, id.data, context, par.data);

                            return_value = 0;
                        }
                    }
                }
            } else if(token.t_state == st_retez) {
                printf("MOVE GF@&pomString string@%s\n", token.t_str.data);
                if((func_return = generate_token())) return func_return;
                if(token.t_state == st_carka) {
                    if((func_return = generate_token())) return func_return;
                    if(token.t_state == st_id) {
                        Tstring par2;
                        str_create_init(&par2, token.t_str.data);
                        if((func_return = generate_token())) return func_return;
                        if(token.t_state == st_pravzav) {
                            if((func_return = generate_token())) return func_return;
                            printf("STRI2INT %s%s GF@&pomString %s%s\n", context, id.data, context, par2.data);

                            str_destroy(&par2);
                            return_value = 0;
                        }
                    } else if(token.t_state == st_int_val) {
                        printf("MOVE GF@&pomInteger int@%s\n", token.t_str.data);
                        if((func_return = generate_token())) return func_return;
                        if(token.t_state == st_pravzav) {
                            if((func_return = generate_token())) return func_return;
                            printf("STRI2INT %s%s GF@&pomString GF@&pomInteger\n", context, id.data);

                            return_value = 0;
                        }
                    }
                }
            }
        }
    }
    else if((func_return = precedent_analysis(instruct, dest_type)) == 0){  //simulace pravidla 22.
        if      (dest_type == st_integer) {
            printf("MOVE %s%s GF@&pomInteger\n",context, id.data);
        } else if (dest_type == st_double) {
            printf("MOVE %s%s GF@&pomFloat\n",context, id.data);
        } else if (dest_type == st_string) {
            printf("MOVE %s%s GF@&pomString\n",context, id.data);
        }

        return_value = 0;
    }
    else return func_return;
    str_destroy(&(fce));
    return return_value;
}// konec funkce rule_assign()

int rule_call_par(){ // stav <call-par>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state == st_id){ // simulace pravidla 24.
        //TODO PARAM
        str_append_str(&param, &(global.current_arguments[index_par - 1].argument_name));
        //printf("%s\n", param.data);
        char* context = (p == 0 ? "TF@" : "LF@");
        printf("DEFVAR %s&arg%d\n", context, arg_cnt);
        printf("MOVE %s&arg%d %s%s\n", context, arg_cnt++, context, token.t_str.data);
        str_destroy(&param);
        if((func_return = generate_token())) return func_return;

        if((return_value = rule_call_next_par()) == 0){
            return_value = 0;
        }
    }

    else if(token.t_state == st_pravzav){ //simulace pravidla 25.
        return_value = 0;
    }

    return return_value;
}

int rule_call_next_par(){ // stav <call-next-par>
    int return_value = ERR_SYN;
    int func_return;
    if(token.t_state == st_carka){ // simulace pravidla 26.
        index_par++;

      if((func_return = generate_token())) return func_return;

        if((return_value = rule_call_par()) == 0){
            return_value = 0;
        }
    }

    else if(token.t_state == st_pravzav){ // simulace pravidla 27.
        return_value = 0;
    }

    return return_value;
}

int rule_pr_expr(){ // stav <pr-expr>
    int return_value = ERR_SYN;
    int func_return;
    Tstate instruct = st_print;
    //printf("%s: ins:%d data: %s\n", "print", instruct, token.t_str.data);
    Tstate dest_type = 0;

    if(token.t_state == st_dim || token.t_state == st_input ||
       token.t_state == st_print || token.t_state == st_if || token.t_state == st_do ||
       token.t_state == st_return || token.t_state == st_end || token.t_state == st_eol ){  // simulace pravidla 31.
            if((func_return = skip_blank_lines())) return func_return;
            return_value = 0;

    } else if((func_return = precedent_analysis(instruct, dest_type)) == 0){  //simulace pravidla 30.
        if(token.t_state == st_stred){
            if (last_gen_type == st_integer) {
                printf("WRITE GF@&pomInteger\n");
            } else if (last_gen_type == st_double) {
                printf("WRITE GF@&pomFloat\n");
            } else if (last_gen_type == st_string) {
                printf("WRITE GF@&pomString\n");
            }
            if((func_return = generate_token())) return func_return;
              if((return_value = rule_pr_expr()) == 0){
                return_value = 0;
            }
        }

    }
    else return func_return;

    return return_value;
}
