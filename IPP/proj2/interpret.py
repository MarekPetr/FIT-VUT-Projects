#!/usr/bin/env python3
# coding=utf-8
from __future__ import print_function
from parser import ParseArgs
from parser import ParseXML
from frames import Frames

import sys
import re
import os
import xml.etree.ElementTree as ET
import operator
import copy

LEX_SYN_ERR          = 32
SEM_ERR              = 52
OPER_TYPE_ERR        = 53
VAR_NOT_EXISTS_ERR   = 54
VALUE_MISSING        = 56 
DIV_BY_ZERO_ERR      = 57
STR_ERR              = 58

# Provede semantickou kontrolu a interpretaci
# vstupem je seznam instrukci (program_list) vygenerovany tridou ParseXML
class Interpret:
    string_reg = re.compile(r'^([^\\#\s]*(\\\d{3})*)*$')
    int_reg = re.compile(r'^([+\-])*(\d)+')

    def __init__(self, program_list):
        # seznam instrukci vstupniho programu
        self.program_list = program_list
        # pocet vsech vykonanych instrukci
        # (muze byt vyssi nez pocet instrukci ve vstupnim programu)
        self.ins_cntr = 0 
        self.prog_cntr = 0 # poradi prave prochazene instrukce od 0

        # seznam slovniku promennych v ramci GF, 
        # docasne tam budou vsechny promenne (TF i LF)
        # slovnik promennych - jmeno:hodnota
        self.frames = Frames(self.prog_cntr)
        self.label_dict = {} # jmeno : poradi_instrukce
        self.data_stack = [] # stack pro POPS a PUSHS
        self.call_stack = []

        # promenna pro skoky
        # - interpret projde znovu program od mista, kam se skoci
        self.again = True

    # hlavni metoda interpretu
    def interpret(self):
        # Projde program a ulozi labely
        for self.prog_cntr, val in enumerate(self.program_list):
            self.opcode = self.program_list[self.prog_cntr][0]
            if self.opcode  == 'LABEL':
                name = self.get_value_prog(1)
                # kdyz neni label ve slovniku labelu, pridej ho tam
                if name not in self.label_dict:
                    self.label_dict[name] = self.prog_cntr
                else:
                    print("{}. instrukce, {}, pokus o redefinici navesti '{}'."
                        .format(self.prog_cntr+1, self.opcode, name), file=sys.stderr)
                    sys.exit(SEM_ERR)

        # Hlavni cyklus vykovanani instrukci       
        self.prog_cntr = 0        
        start = 0 # misto kam se ma skocit
        self.again = True        
        while self.again:
            self.prog_cntr = 0
            self.again = False

            for self.prog_cntr, val in enumerate(self.program_list[start:], start = start):
                #print(self.prog_cntr, val) #DEBUG         
                #print(program_list[self.prog_cntr][0]) #DEBUG 
                self.opcode = self.program_list[self.prog_cntr][0]
                self.ins_cntr += 1
                self.frames.update_prog_cntr(self.prog_cntr)
                if self.opcode == 'LABEL':
                    pass
                ###############################################################
                elif self.opcode == 'DEFVAR':
                    var = self.get_var_prog()
                    self.frames.get_frame(var) # zkontroluje jestli frame existuje
                    # deklarace promenne, ktera jeste neni deklarovana
                    if not self.is_var_declared(var):
                        self.declare_var(var)
                ###############################################################
                elif self.opcode == 'MOVE':
                    symb_type = self.get_symb_type(2, True)
                    value = self.get_symb_value(2)                    
                    self.update_or_def_val(1, symb_type, value)
                ###############################################################
                elif self.opcode == 'JUMP':
                    start = self.jump()
                    if self.again:
                        break
                ###############################################################
                elif self.opcode == 'JUMPIFEQ':
                    start = self.jump_equality(operator.eq)
                    if self.again:
                        break
                ###############################################################
                elif self.opcode == 'JUMPIFNEQ':
                    start = self.jump_equality(operator.ne)
                    if self.again:
                        break
                ###############################################################
                elif self.opcode == 'WRITE':
                    symb_type = self.get_symb_type(1, True)
                    value = self.get_symb_value(1)                    
                    value = self.bool_to_str(symb_type, value)
                    print(value, file=sys.stdout)
                elif self.opcode == 'DPRINT':
                    pass
                ###############################################################
                elif self.opcode == 'CONCAT':
                    self.operator_3_args(operator.add,'string')
                elif self.opcode == 'ADD':
                    self.operator_3_args(operator.add,'int')
                elif self.opcode == 'SUB':
                    self.operator_3_args(operator.sub,'int')
                elif self.opcode == 'MUL':
                    self.operator_3_args(operator.mul,'int')
                elif self.opcode == 'IDIV':
                    self.operator_3_args(operator.floordiv,'int')
                elif self.opcode == 'LT':
                    self.operator_3_args(operator.lt,'any')
                elif self.opcode == 'GT':
                    self.operator_3_args(operator.gt,'any')
                elif self.opcode == 'EQ':
                    self.operator_3_args(operator.eq,'any')
                elif self.opcode == 'AND':
                    self.operator_3_args(operator.and_,'bool')
                elif self.opcode == 'OR':
                    self.operator_3_args(operator.or_,'bool')                
                ###############################################################
                elif self.opcode == 'NOT':
                    self.op_not()
                elif self.opcode == 'PUSHS':
                    symb_type = self.get_symb_type(1, True)
                    value = self.get_symb_value(1)
                    var_list = []
                    var_list.append(symb_type)
                    var_list.append(value)
                    self.data_stack.append(var_list)
                ###############################################################
                elif self.opcode == 'POPS':
                    stack_len = len(self.data_stack)
                    if stack_len > 0:
                        value_list = self.data_stack.pop()
                        var_type = value_list[0]
                        value = value_list[1]
                        self.update_or_def_val(1, var_type, value)
                    else:
                        print("{}. instrukce, {}, datovy zasobnik je prazdny"
                            .format(self.prog_cntr+1, self.opcode), file=sys.stderr)
                        sys.exit(VALUE_MISSING)
                ###############################################################  
                elif self.opcode == 'TYPE':
                    arg_type = self.get_type_prog(2)
                    name = self.get_value_prog(2)
                    if arg_type == 'var':
                        if not self.is_var_declared(name):
                            self.not_declared_err(name)

                    # promenna nemusi byt definovana -> False
                    value = self.get_symb_type(2, False)
                    self.update_or_def_val(1, 'string', value)
                ###############################################################
                elif self.opcode == 'READ':
                    exp_type = self.check_type_get_value(2, 'type')
                    try:
                        value = input()
                    except EOFError as error:
                        value = ''

                    if exp_type == 'int':
                        if re.match(self.int_reg, value):
                            value = int(value)
                        else:
                            value = 0
                    elif exp_type == 'bool':
                        val_lower = value.lower()
                        if val_lower in {'true','false'}:
                            value = self.str_to_bool('bool',val_lower)
                        else:
                            value = False

                    self.update_or_def_val(1, exp_type, value)
                ###############################################################
                elif self.opcode == 'STRLEN':
                    value = self.check_type_get_value(2, 'string')
                    length = len(value)
                    self.update_or_def_val(1, 'int', length)
                ###############################################################
                elif self.opcode == 'STRI2INT':
                    string = self.check_type_get_value(2, 'string')
                    index = self.check_type_get_value(3, 'int')
                    self.check_str_bounds(index, string)
                    char = string[index]
                    try:
                        ord_val = ord(char)
                    except TypeError as error:
                        print("{}. instrukce, {}, znak {} se nepodaril prevest na ordinalni hodnotu"
                            .format(self.prog_cntr+1, self.opcode, char), file=sys.stderr)
                        sys.exit(STR_ERR)
                    self.update_or_def_val(1, 'int', ord_val)
                ###############################################################
                elif self.opcode == 'INT2CHAR':
                    value = self.check_type_get_value(2, 'int')
                    try:
                        char = chr(value)
                    except ValueError as error:
                        print("{}. instrukce, {}, hodnota {} neni validni ordinalni hodnota unicode"
                            .format(self.prog_cntr+1, self.opcode, value), file=sys.stderr)
                        sys.exit(STR_ERR)
                    self.update_or_def_val(1, 'string', char)
                ###############################################################
                elif self.opcode == 'GETCHAR':
                    symb1_type = self.get_symb_type(2, True)
                    if symb1_type != 'string':
                        self.symb_type_err(2, 'string')
                    
                    symb2_type = self.get_symb_type(3, True)
                    if symb2_type != 'int':
                        self.symb_type_err(3, 'int')

                    string = self.get_symb_value(2)
                    index = self.get_symb_value(3)
                    self.check_str_bounds(index, string)
                                           
                    char = string[index]
                    self.update_or_def_val(1, 'string', char)
                ###############################################################
                elif self.opcode == 'SETCHAR':
                    to_change = self.check_type_get_value(1, 'string')

                    symb1_type = self.get_symb_type(2, True)
                    if symb1_type != 'int':
                        self.symb_type_err(2, 'int')

                    string = self.check_type_get_value(3, 'string')
                    if len(string) > 0:
                        char = string[0]
                    else:
                        print("{}.instrukce, {}, retezec ve 3. argumentu je prazdny"
                            .format(self.prog_cntr+1, self.opcode), file=sys.stderr)
                        sys.exit(STR_ERR)

                    index = self.get_symb_value(2)                    
                    self.check_str_bounds(index, to_change)
                    changed_str = to_change[:index] + char + to_change[index+1:]
                    self.update_or_def_val(1, 'string', changed_str)
                ###############################################################

                elif self.opcode == 'BREAK':
                    print("Pozice v kodu: {}. instrukce, {}, pocet vykonanych instrukci: {}."
                        .format(self.prog_cntr+1, self.opcode, self.ins_cntr), file=sys.stderr)
                    print("Definovana navesti:\n{}"
                        .format(self.label_dict), file=sys.stderr)
                    print("\nGlobalni ramec:\n{}"
                        .format(self.frames.return_frame('GF')), file = sys.stderr)
                    print("\nLokalni ramec:\n{}"
                        .format(self.frames.return_frame('LF')), file = sys.stderr)
                    print("\nDocasny ramec:\n{}"
                        .format(self.frames.return_frame('TF')), file = sys.stderr)
                    print("\nDeklarovane promenne:\n{}"
                        .format(self.var_name_list), file = sys.stderr)
                    print("\nDatovy zasobnik:\n{}"
                        .format(self.data_stack), file = sys.stderr)

                ###############################################################
                elif self.opcode == 'CREATEFRAME':
                    self.frames.create_frame()
                elif self.opcode == 'PUSHFRAME':
                    self.frames.push_frame()
                elif self.opcode == 'POPFRAME':
                    self.frames.pop_frame()
                ###############################################################
                elif self.opcode == 'CALL':
                    label = self.check_type_get_value(1, 'label')
                    self.call_stack.append(self.prog_cntr)

                    start = self.jump()
                    if self.again:
                        break

                elif self.opcode == 'RETURN':
                    # skoci az za CALL, jinak by se zacyklil
                    if len(self.call_stack) > 0:
                        start = self.call_stack.pop() + 1
                        self.again = True
                        break
                    else:
                        print("{}. instrukce, {}, zasobnik volani je prazdny"
                            .format(self.prog_cntr+1, self.opcode), file=sys.stderr)
                        sys.exit(VALUE_MISSING)
                else:
                    print("{}. instrukce, neznama instrukce {}"
                        .format(self.prog_cntr+1, self.opcode), file=sys.stderr)
                    sys.exit(LEX_SYN_ERR)

    ########################################################################
    ########################################################################
    # vykona instrukce se tremi argumenty, ktere vyzaduji pouziti nejakeho operatoru
    # operator: definuje operaci mezi symboly
    # arg_type: podporovany typ vsech argumentu 
    # (tam kde jsou libovolne je tato hodnota 'any')
    def operator_3_args(self, operator, arg_type):
        symb1_type = self.get_symb_type(2, True)
        symb2_type = self.get_symb_type(3, True)
        symb1_val = self.get_symb_value(2)
        symb2_val = self.get_symb_value(3)

        defined = False
        if arg_type != 'any':
            if symb1_type != arg_type or symb2_type != arg_type:
                self.types_err(arg_type)
        else:
            if symb1_type != symb2_type:
                self.same_types_err()
            arg_type = 'bool'

        try:
            value = operator(symb1_val, symb2_val)
        except ZeroDivisionError as error:
            print("{}.instrukce, {}, deleni nulou."
                .format(self.prog_cntr+1, self.opcode))
            sys.exit(DIV_BY_ZERO_ERR)
        # je potreba, protoze v ostatnich pripadech se hodnota ziskava ve forme retezce
        self.update_or_def_val(1, arg_type, value) 

    # vykona instrukci NOT
    def op_not(self):
        symb_type = self.get_symb_type(2, True)
        if symb_type != 'bool':
            self.types_err('bool')
        symb_val = self.get_symb_value(2)
        value = not symb_val
        self.update_or_def_val(1, 'bool', value) 

    # skoci na label
    def jump(self):
        start = 0
        label = self.get_value_prog(1)
        if label not in self.label_dict:
            print("{}. instrukce, {}, skok na neexistujici navesti: {}."
                .format(self.prog_cntr+1, self.opcode, label), file=sys.stderr)
            sys.exit(SEM_ERR)
        else:
             # skoci az za label, aby nenastala chyba dvojite deklarace labelu
            start = self.label_dict[label]+1
            self.again = True
        return start

    # skoci na label v pripade, kdy vysledna hodnota vyrazu je vyhodnocena jako true
    # operator - operace mezi temito dvema symboly
    def jump_equality(self, operator):
        start = 0
        symb1_type = self.get_symb_type(2, True)
        symb2_type = self.get_symb_type(3, True)
        symb1_val = self.get_symb_value(2)
        symb2_val = self.get_symb_value(3)

        if symb1_type == symb2_type:
            if operator(symb1_val, symb2_val):
                start = self.jump()                
        else:
            print("{}.instrukce, {}, nelze porovnat typ {} s {}."
                .format(self.prog_cntr+1, self.opcode, symb1_type, symb2_type), file=sys.stderr)
            sys.exit(OPER_TYPE_ERR)
        return start
    ########################################################################
    # ze vstupniho seznamu instrukci vraci nazev promenne (tedy prvni argument) dane instrukce
    # napr z ['MOVE', {'var': 'GF@counter'}, {'string': None}] vrati GF@counter
    def get_var_prog(self):
        # var_dict - 'var' : jmeno
        var_dict = self.program_list[self.prog_cntr][1]
        var = var_dict['var']
        return var

    # Ze vstupniho seznamu instrukci vraci typ argumentu
    def get_type_prog(self, arg_order):
        type_dict = self.program_list[self.prog_cntr][arg_order]
        if 'var' in type_dict:
            var_type = 'var'
        elif 'int' in type_dict:
            var_type = 'int'
        elif 'string' in type_dict:
            var_type = 'string'
        elif 'bool' in type_dict:
            var_type = 'bool'
        elif 'label' in type_dict:
            var_type = 'label'
        elif 'type' in type_dict:
            var_type = 'type'
        else:
            print("Neexistujici typ.",file=sys.stderr)
            sys.exit(LEX_SYN_ERR)
        return var_type

    # Ze vstupniho seznamu instrukci vraci hodnotu argumentu
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    def get_value_prog(self,arg_order):
        var_type = self.get_type_prog(arg_order)
        value_dict = self.program_list[self.prog_cntr][arg_order]
        value = value_dict[var_type]
        value = self.str_to_int(var_type, value)             
        value = self.str_to_bool(var_type, value)

        if var_type == 'string':
            if value is None:
                value = ''
            else:
                value = self.convert_esc_seq(value)
        return value
    ########################################################################

    # vraci typ argumentu ze slovniku promenne (ze seznamu ramce)
    # var_dict - slovnik promenne: {jmeno : typ}
    def var_type_dict(self, var_dict):
        if 'int' in var_dict:
            var_type = 'int'
        elif 'string' in var_dict:
            var_type = 'string'
        elif 'bool' in var_dict:
            var_type = 'bool'
        elif 'type' in var_dict:
            var_type = 'type'
        else:
            print("Neexistujici typ promenne.",file=sys.stderr)
            sys.exit(LEX_SYN_ERR)
        return var_type

    # vraci hodnotu promenne ze slovniku promenne
    # var_dict - slovnik promenne: {jmeno : typ}
    def get_var_value(self, var_dict):
        var_type = self.var_type_dict(var_dict)        
        value = var_dict[var_type]            
        return value

    # konvertuje escape sekvenci na ASCII znak
    def convert_esc_seq(self, value):
        new_str = value
        esc_reg = re.compile(r'\\\d{3}')
        all_esc = re.findall(esc_reg, value)
        for esc in all_esc:
            try:
                string = chr(int(esc.lstrip('\\')))
                new_str = new_str.replace(esc, string)
            except TypeError as error:
                print("{}.instrukce, {}, escape sekvence {} se nepodarila prevest na znak unicode."
                    .format(self.prog_cntr+1, self.opcode, esc), file=sys.stderr)
                sys.exit(LEX_SYN_ERR)            
        return new_str

    ########################################################################
    
    # V programu na indexu instrukce self.prog_cntr najde argument v poradi arg_order
    # a vrati jeho hodnotu.
    # V pripade promenne vraci hodnotu promenne z daneho framu (listu).
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    def get_value(self, arg_order):
        var_type = self.get_type_prog(arg_order)
        value = self.get_value_prog(arg_order)
        
        if var_type == 'var':
            frame = self.frames.get_frame(value)
            value = self.without_frame(value)
            defined = False
            for var_list in frame:
                if value in var_list:
                    if len(var_list) != 2:
                        self.not_defined_err(value)
                    var_dict = var_list[1]
                    value = self.get_var_value(var_dict)
                    defined = True
                    break
            if not defined:
                self.not_defined_err(value)
        return value

    # v danem ramci aktualizuje hodnotu a typ promenne
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    # var_dict - slovnik promenne: {jmeno : typ}
    def update_value(self, arg_order, var_dict):
        symb_type = self.get_type_prog(arg_order)
        if symb_type != 'var':
            return False
        
        name = self.get_value_prog(arg_order)
        
        frame = self.frames.get_frame(name)
        name = self.without_frame(name)

        defined = False
        for var_list in frame:
            if var_list[0] == name:
                if len(var_list) == 2:
                    var_list.pop()                
                var_list.append(var_dict)
                defined = True
        
        if not defined:
            self.not_defined_err(name)

    # vraci typ promenne z jejiho ramce
    # v pripade instrukce TYPE je pri nenalezeni typu vracen prazdny retezec
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    def get_var_type(self, arg_order, exit_error):
        #              var       symb_type  value      
        #frame = [['GF@retezec', {'string': ahoj}]]
        #           |              |--------------|| var_dict
        #           |------------------------------| var_list
        symb_type = self.get_type_prog(arg_order)
        if symb_type != 'var':
            return False
        
        name = self.get_value_prog(arg_order)
        frame = self.frames.get_frame(name)
        name = self.without_frame(name)

        for var_list in frame:
            if var_list[0] == name:
                if len(var_list) == 2:
                    var_dict = var_list[1]
                    ret_type = self.var_type_dict(var_dict)
                else:
                    self.not_defined_err(name)
                return ret_type

        if exit_error:
            self.not_defined_err(name)
        else:
            return '' # pro instrukci TYPE

    # vrati hodnotu symbolu (promenne i konstanty)
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    def get_symb_value(self, arg_order):
        symb_type = self.get_type_prog(arg_order)
        if symb_type == 'var':
            symb_val = self.get_value(arg_order)
        else:
            symb_val = self.get_value_prog(arg_order)
        return symb_val

    # vrati typ symbolu
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    def get_symb_type(self,arg_order, exit_error):
        symb_type = self.get_type_prog(arg_order)
        if symb_type == 'var':
            symb_type = self.get_var_type(arg_order, exit_error)
        return symb_type

    # zjisti typ symbolu a zkontroluje, jestli je spravny
    # v pripade uspechu vraci hodnotu tohoto symbolu
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    # exp_type - spravny (ocekavany) typ symbolu
    def check_type_get_value(self, arg_order, exp_type):
        symb_type = self.get_symb_type(arg_order, True)
        if symb_type != exp_type:
            self.symb_type_err(arg_order, exp_type)
        else:
            return self.get_symb_value(arg_order)    

    # v pripade definovane promenne ji aktualizuje, jinak definuje
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    def update_or_def_val(self, arg_order, exp_type, value):
        defined = False
        if self.is_var_defined(arg_order):
            var_dict = {exp_type : value}
            self.update_value(arg_order, var_dict)
        else:
            self.define_undecl_var(arg_order, exp_type, value)

    # vraci True pokud je promenna definovana, jinak False
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    def is_var_defined(self, arg_order):
        name = self.get_value_prog(arg_order)
        frame = self.frames.get_frame(name)
        name = self.without_frame(name)        

        for var_list in frame:
            if var_list[0] == name and len(var_list) == 2:
                return True
        return False

    # z promenne odstrani prvni tri znaky (tedy GF@)
    # pozor na nazvy promennych zacinajici na G|T|LF@
    #  - tato hodnota by byla take odstranena
    # var - nazev promenne
    def without_frame(self, var):
        if var.startswith('GF@') or var.startswith('LF@') or var.startswith('TF@'):
            return var[3:]

    # najde index promenne v danem ramci
    # v pripade neuspechu vraci -1
    # var - nazev promenne
    # frame - ramec (seznam) 
    def find_var_index(self, var, frame):
        for index, var_list in enumerate(frame):
            if var_list[0] == var:
                return index
        return -1


    # definuje promennou
    # var - jmeno promenne
    # symb_type - typ promenne
    # value - prirazena hodnota
    def define_var(self, var, symb_type, value):
        frame = self.frames.get_frame(var)
        var = self.without_frame(var)

        index = self.find_var_index(var, frame)
        var_dict = {symb_type : value}
        frame[index].append(var_dict)

    # definuje deklarovanou promennou    
    # arg_order - poradi argumentu prave vyhodnocovane instrukce
    # var_type - typ promenne
    # value - hodnota
    def define_undecl_var(self, arg_order, var_type, value):
        var = self.get_var_prog()
        if self.is_var_declared(var):
            self.define_var(var, var_type, value)
        else:
            self.not_declared_err(var)
    
    ########################################################################

    # ladici vystup pro vypis vstupniho programu
    def print_program(self):
        for index, val in enumerate(self.program_list):
            print(index, val)
        print("--------------------------------------------------------")

    # v pripade, ze je promenna var deklarovana, vraci True, jinak False
    # var - nazev promenne i s ramcem
    def is_var_declared(self, var):        
        frame = self.frames.get_frame(var)
        var = self.without_frame(var)

        index = self.find_var_index(var, frame)
        if index == -1:
            return False
        else:
            return True

    # deklaruje promennou
    # var - nazev promenne i s ramcem
    def declare_var(self,var):
        frame = self.frames.get_frame(var)
        var = self.without_frame(var)

        var_list = []
        var_list.append(var)
        frame.append(var_list)

    # konvertuje bool hodnotu IPPcode18 na bool hodnoty pythonu
    # arg_type - typ symbolu
    # value - hodnota 
    def str_to_bool(self, arg_type, value):
        if arg_type == 'bool':            
            if value == 'true':
                value = True
            else:
                value = False
        return value

    # konvertuje bool hodnotu pythonu na hodnotu IPPcode18
    # arg_type - typ symbolu
    # value - hodnota 
    def bool_to_str(self, arg_type, value):
        if arg_type == 'bool':
            if value == True:
                value = 'true'
            else:
                value = 'false'
        return value

    # prevede cislo ulozene v retezci na cele cislo srozumitelne pythonem
    # arg_type - typ symbolu
    # value - hodnota  
    def str_to_int(self, arg_type, value):
        if arg_type == 'int':
            value = int(value)
        return value

    # zkontroluje, jestli se na indexu v retezci naleza nejaka hodnota
    # index - poradi znaku v retezci (od 0)
    # string - retezec
    def check_str_bounds(self, index, string):
        length = len(string)
        if index >= length or index < 0:
            print("{}.instrukce, {}, index {} je mimo velikost retezce {}"
                .format(self.prog_cntr+1, self.opcode, index, string), file=sys.stderr)
            sys.exit(STR_ERR)

    ########################################################################################
    # vypise chybove hlaseni spatneho typu symbolu a skonci s chybou
    # arg_order - poradi argumentu v instrukci
    # symb_type - typ symbolu
    def symb_type_err(self, arg_order, symb_type):
        print("{}.instrukce, {}, {}. argument musi byt typu {}."
            .format(self.prog_cntr+1, self.opcode, arg_order, symb_type), file=sys.stderr)
        sys.exit(OPER_TYPE_ERR)

    # vypise chybove hlaseni nedefinovane promenne a skonci s chybou
    # var - jmeno promenne
    def not_defined_err(self, var):
        print("{}. instrukce, {}, promenna {} neni definovana."
            .format(self.prog_cntr+1, self.opcode, var), file=sys.stderr)
        sys.exit(VALUE_MISSING)

    # vypise chybove hlaseni nedeklarovane promenne a skonci s chybou
    # var - jmeno promenne
    def not_declared_err(self,var):
        print("{}. instrukce, {}, promenna {} neni deklarovana."
                    .format(self.prog_cntr+1, self.opcode, var), file=sys.stderr)
        sys.exit(VAR_NOT_EXISTS_ERR)

    # vypise chybove hlaseni nekompatibility typu s instrukci
    # symb_type - typ symbolu
    def types_err(self, symb_type):
        print("{}.instrukce, {} podporuje pouze typ {}."
            .format(self.prog_cntr+1, self.opcode, symb_type), file=sys.stderr)
        sys.exit(OPER_TYPE_ERR)

    # vypise chybove hlaseni v pripade odlisnych typu instrukce vyzadujici stejne typy symbolu (napr. ADD)
    def same_types_err(self):
        print("{}.instrukce, symboly {} musi byt stejneho typu."
            .format(self.prog_cntr+1, self.opcode), file=sys.stderr)
        sys.exit(OPER_TYPE_ERR)
    ########################################################################################


# HLAVNI TELO PROGRAMU
args_obj = ParseArgs()
xml_file = args_obj.parse_args()

xml_obj = ParseXML(xml_file)
program_list = xml_obj.parse_file()

int_obj = Interpret(program_list)
int_obj.interpret()
