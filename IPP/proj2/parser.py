#!/usr/bin/env python3
# coding=utf-8
from __future__ import print_function
import sys
import re
import os
import xml.etree.ElementTree as ET

ARG_ERR        = 10
IN_FILE_ERR    = 11
OUT_FILE_ERR   = 12
XML_ERR        = 31
LEX_SYN_ERR    = 32
SEM_ERR        = 52

# parsuje vstupni argumenty programu 
class ParseArgs:
    def parse_args(self):
        arg_cnt = len(sys.argv)
        if arg_cnt == 2:
            matchObj = re.match('--source=',sys.argv[1])
            if matchObj:
                 # odstrani z argumentu '--source=', zustane soubor
                file = re.sub('--source=','',sys.argv[1])
                if not file: #kontrola na prazdny string
                    print("V parametru --source chybi nazev souboru.", file=sys.stderr)
                    sys.exit(ARG_ERR)
            else:
                if '--help' == sys.argv[1]:
                    print("help")
                    sys.exit(0)
                else:
                    print("Script musi byt spusten s parametry --help nebo --source.", file=sys.stderr)
                    sys.exit(ARG_ERR)
        else:
            print("Script musi byt spusten s parametry --help nebo --source.", file=sys.stderr)
            sys.exit(ARG_ERR)

        if not os.path.isfile(file):
            print("Soubor {} neexistuje.". format(file), file=sys.stderr)
            sys.exit(IN_FILE_ERR)
        return file

# Nejprve provede testy formatu vstupniho XML souboru.
# Pote provede lexikalni a syntaktickou analyzu instrukci
class ParseXML:
    var_reg = re.compile(r'^[TLG]F@[a-zA-Z_\-$&%\*]([a-zA-Z\d_\-$&%\*])*$')
    label_reg = re.compile(r'^([a-zA-Z_\-$&%\*][a-zA-Z\d_\-$&%\*]*)$')
    #string_reg = re.compile(r'^([^\\#\s]*(\\\d{3}(?!\d))*)*$')
    string_reg = re.compile(r'^([^\\#\s]*(\\\d{3})*)*$')
    int_reg = re.compile(r'^([+\-])*(\d)+')
    label_reg = re.compile(r'^([a-zA-Z_\-$&%*][a-zA-Z\d_\-$&%*]*)$')
    
    ins_order = 0 # poradi prave kontrolovane instrukce

    program_list = []
    instr_list = []

    def __init__(self, file_name):
        self.file_name = file_name

    # hlavni metoda tridy ParseXML
    def parse_file(self):
        try:
            tree = ET.parse(self.file_name)
        except:
            print("Spatny format vstupniho XML souboru.", file=sys.stderr)
            sys.exit(XML_ERR)

        program = tree.getroot()
        if program.tag != "program":
            print("Korenovy element 'program' nenalezen.", file=sys.stderr)
            sys.exit(XML_ERR)
        
        # zjisti, jestli existuje atribut 'language' a priradi jeho hodnotu do lang
        lang = program.attrib.get('language',False)
        if lang == False:
            print("Atribut 'language' nebyl nalezen.", file=sys.stderr)
            sys.exit(XML_ERR)
        elif lang != 'IPPcode18':
            print("Hodnota atributu 'language' neni IPPcode18.", file=sys.stderr)
            sys.exit(XML_ERR)

        exp_prog_cnt = 1

        name = program.attrib.get('name',False)
        if name != False:
            exp_prog_cnt += 1

        descript = program.attrib.get('description',False)
        if descript != False:
            exp_prog_cnt += 1

        # pocet atributu programu
        cnt = len(program.attrib)
        if cnt != exp_prog_cnt:
            print("Spatne atributy programu.", file=sys.stderr)
            sys.exit(XML_ERR)

        ref_order = 0;
        for instruct in program:
            ref_order += 1
            if instruct.tag != 'instruction':
                print("Chybi element 'instruction'.", file=sys.stderr);
                sys.exit(XML_ERR)

            #print (instruct.tag, instruct.attrib) #DEBUG
            # zkontroluje poradi ins_order
            ins_order = instruct.attrib.get('order', False)
            if ins_order == False:
                print("Atribut 'order' nebyl nalezen.", file=sys.stderr)
                sys.exit(XML_ERR)
            else:
                ins_order = int(ins_order)            
                cnt = len(instruct.attrib)
                if cnt != 2:
                    print("{}. instrukce, element 'instruction' musi mit dva atributy."
                        .format(ins_order), file=sys.stderr)
                    sys.exit(XML_ERR)
                if ins_order != ref_order:
                    print("Atribut order je {}, ale ma byt {}."
                        .format(ins_order, ref_order), file=sys.stderr)
                    sys.exit(XML_ERR)
                self.ins_order = ins_order;

            # zkontroluje opcode a atributy
            opcode = instruct.attrib.get('opcode', False)

            if opcode == False:
                print("{}. instrukce, atribut opcode nebyl nalezen.".format(self.ins_order), file=sys.stderr)
                sys.exit(XML_ERR)
            
            # do seznamu instrukce prida jeji nazev
            self.instr_list.append(opcode)

             # kontrola OPCODE
            arg_cnt = len(instruct.getchildren())            
            
            exp_type = 'any'
            if opcode in {"PUSHFRAME","CREATEFRAME","POPFRAME","RETURN","BREAK"}:
                self.check_args_cnt(opcode, arg_cnt, 0)

            elif opcode in {"DEFVAR", "POPS"}:
                self.check_args_cnt(opcode, arg_cnt, 1)
                arg1 = self.find_arg(instruct, 'arg1')
                self.check_var(opcode, arg1)

            elif opcode in {"CALL", "LABEL", "JUMP"}:                
                self.check_args_cnt(opcode, arg_cnt, 1)
                arg1 = self.find_arg(instruct, 'arg1')
                self.check_label(opcode, arg1)

            elif opcode in {"PUSHS", "WRITE", "DPRINT"}:
                self.check_args_cnt(opcode, arg_cnt, 1)
                arg1 = self.find_arg(instruct, 'arg1')
                self.check_symbol(opcode, arg1, exp_type)

            elif opcode in {"MOVE", "INT2CHAR", "NOT", "STRLEN", "TYPE"}:                
                self.check_args_cnt(opcode, arg_cnt, 2)
                arg1 = self.find_arg(instruct, 'arg1')                
                arg2 = self.find_arg(instruct, 'arg2')
                self.check_var(opcode, arg1)
                if opcode == 'INT2CHAR':
                    exp_type = 'int'
                elif opcode == 'STRLEN':
                    exp_type = 'string'
                elif opcode == 'NOT':
                    exp_type = 'bool'
                self.check_symbol(opcode, arg2, exp_type) 

            elif opcode in {"READ"}:
                self.check_args_cnt(opcode, arg_cnt, 2)
                arg1 = self.find_arg(instruct, 'arg1')                
                arg2 = self.find_arg(instruct, 'arg2')
                self.check_var(opcode, arg1)
                self.check_type_type(opcode, arg2)

            elif opcode in {"ADD", "SUB", "MUL", "IDIV", "LT", "GT",
                            "EQ", "AND", "OR", "STRI2INT", "CONCAT", 
                            "GETCHAR", "SETCHAR"}:
                self.check_args_cnt(opcode, arg_cnt, 3)
                arg1 = self.find_arg(instruct, 'arg1')
                self.check_var(opcode, arg1)
                
                arg2 = self.find_arg(instruct, 'arg2')
                arg3 = self.find_arg(instruct, 'arg3')                
                
                if opcode in {"ADD", "SUB", "MUL", "IDIV"}:
                    exp_type = "int"
                elif opcode in {"AND", "OR"}:
                    exp_type = 'bool'
                elif opcode in {'STRI2INT', 'CONCAT', 'GETCHAR'}:
                    exp_type = 'string'
                elif opcode == 'SETCHAR':
                    exp_type = 'int'
                
                self.check_symbol(opcode, arg2, exp_type)

                if opcode in {'STRI2INT', 'GETCHAR'}:
                    exp_type = 'int'
                elif opcode == 'SETCHAR':
                    exp_type = 'string'
                self.check_symbol(opcode, arg3, exp_type)

            elif opcode in {"JUMPIFEQ", "JUMPIFNEQ"}:
                self.check_args_cnt(opcode, arg_cnt, 3)
                arg1 = self.find_arg(instruct, 'arg1')
                arg2 = self.find_arg(instruct, 'arg2')                
                arg3 = self.find_arg(instruct, 'arg3')
                self.check_label(opcode, arg1)
                self.check_symbol(opcode, arg2, exp_type)
                self.check_symbol(opcode, arg3, exp_type)
            else:
                print("Neznama instrukce: {}.".format(opcode), file=sys.stderr)
                sys.exit(LEX_SYN_ERR)            
            
            self.program_list.append(self.instr_list) # do seznamu program prida instrukci
            self.instr_list = [] #instrukci vyprazdni

        return self.program_list
    
    # vyhleda argument instrukce (xml souboru)
    # instruct - instrukce
    # name - jmeno argumentu (arg1/2/3)
    def find_arg(self, instruct, name):
        arg = instruct.find(name)
        if arg == None:
            print("{}. Instrukce, atribut {} nenalezen."
                .format(self.ins_order, name), file=sys.stderr)
            sys.exit(LEX_SYN_ERR)
        else:
            return arg

    # zkontroluje, jestli ma instrukce spravny pocet argumentu
    # opcode - operacni kod instrukce
    # arg_curr - aktualni pocet argumentu
    # arg_expec - pozadovany pocet argumentu
    def check_args_cnt(self, opcode, arg_curr, arg_expec):
        if arg_curr != arg_expec:
            print("{}. Instrukce, {} musi mit {} argumentu, ale ma {}."
                .format(self.ins_order, opcode, arg_expec, arg_curr), file=sys.stderr)
            sys.exit(LEX_SYN_ERR)

    # zkontroluje, jestli ma element 'arg' spravny pocet atributu - tedy jeden
    # arg - element argumentu
    def check_arg_attrib_cnt(self, arg):
        arg_attrib_cnt = len(arg.attrib)
        if arg_attrib_cnt != 1:
            print("{}. instrukce, spatne atributy argumentu {}."
                .format(self.ins_order, arg.tag), file=sys.stderr)
            sys.exit(XML_ERR)
    
    # zkontroluje, jestli ma element attribut 'type' a jestli je typ spravny 
    # arg - element argumentu
    # exp_type - ocekavany typ
    # opcode - operacni kod instrukce
    def check_arg_attrib(self, arg, exp_type, opcode):
        arg_type = arg.attrib.get('type', False)
        if arg_type == False:
            print("{}. instrukce, atribut instrukce 'type' nebyl nalezen."
                .format(self.ins_order), file=sys.stderr)
            sys.exit(XML_ERR)
        if arg_type != exp_type:
            print("{}. instrukce, argument {} instrukce '{}' musi byt '{}'."
                .format(self.ins_order, arg.tag, opcode, exp_type), file=sys.stderr)
            sys.exit(LEX_SYN_ERR)

    # zkontroluje, jestli ma argument instrukce spravny typ (vsechny krome symbolu a bool)
    # arg - element argumentu
    # arg_type - typ argumentu
    def check_type(self, regex, arg, arg_type):
        if arg.text is None:
            arg.text = ''
        if not re.match(regex, arg.text):
            print("{}. instrukce, spatny typ argumentu, '{}' neni {}."
                .format(self.ins_order, arg.text, arg_type), file=sys.stderr);
            sys.exit(LEX_SYN_ERR)

    # zjisti, jestli ma argument instrukce dany typ
    # regex - regularni vyraz pro test typu
    # arg - element argumentu
    def is_specif_type(self, regex, arg):
        if arg.text is None:
            arg.text = ''
        if not re.match(regex, arg.text):
            return False
        return True

    # zkontroluje, jestli ma argument instrukce textovy element
    # pokud ano, appenduje argumenty instrukce do listu self.instr_list
    # arg - element argumentu
    def check_arg_text(self, arg):
        arg_type = arg.attrib.get('type', False)
        if arg.text is None and arg_type != 'string':
            print("{}. instrukce, hodnota argumentu '{}' nemuze byt prazdna."
                .format(self.ins_order, arg_type))
            sys.exit(LEX_SYN_ERR)
        
        # do instrukce prida dalsi argumenty instrukce
        arg_dict = {arg_type : arg.text}
        self.instr_list.append(arg_dict)

    # zkontroluje, jestli ma argument instrukce spravny xml zapis
    # arg - element argumentu
    # opcode - operacni kod instrukce 
    def check_arg(self, opcode, arg):
        self.check_arg_attrib_cnt(arg)
        self.check_arg_text(arg)

    # zkontroluje, jestli je dany argument instrukce promenna
    # opcode - operacni kod instrukce
    # arg - element argumentu
    def check_var(self, opcode, arg):
        exp_type = 'var'
        self.check_arg(opcode, arg)
        self.check_arg_attrib(arg, exp_type, opcode)
        self.check_type(self.var_reg,arg,exp_type)

        
    # zkontroluje, jestli je dany argument instrukce promenna
    # opcode - operacni kod instrukce
    # arg - element argumentu
    def check_label(self, opcode, arg):
        exp_type = 'label'
        self.check_arg(opcode, arg)
        self.check_arg_attrib(arg, exp_type, opcode)
        self.check_type(self.label_reg,arg,exp_type)

    # zkontroluje, jestli je dany argument instrukce retezec
    # opcode - operacni kod instrukce
    # arg - element argumentu
    def check_string(self, opcode, arg):
        exp_type = 'string'
        self.check_arg(opcode, arg)
        self.check_arg_attrib(arg, exp_type, opcode)
        self.check_type(self.string_reg, arg, exp_type)

    # zkontroluje, jestli je dany argument instrukce bool
    # opcode - operacni kod instrukce
    # arg - element argumentu
    def check_bool(self, opcode, arg):
        exp_type = 'bool'
        self.check_arg(opcode, arg)
        self.check_arg_attrib(arg, exp_type, opcode)
        if arg.text not in {'true', 'false'}:
            print("{}. instrukce, typ bool u instrukce {} muze byt pouze 'true' nebo 'false'."
                .format(arg.tag), file=sys.stderr)
            sys.exit(XML_ERR)

    # zkontroluje, jestli je dany argument instrukce type
    # opcode - operacni kod instrukce
    # arg - element argumentu
    def check_type_type(self, opcode, arg):
        exp_type = 'type'
        self.check_arg(opcode, arg)
        self.check_arg_attrib(arg, exp_type, opcode)
        if arg.text not in {'int', 'bool', 'string'}:
            print("{}. instrukce, spatny format typu '{}'".
                format(self.ins_order, arg.text), file=sys.stderr);
            sys.exit(XML_ERR)

    # vrati typ argumentu instrukce (pouziti u symbolu), 
    # pokud jde o nektery ze symbolu, jinak skonci s chybou
    # arg - element argumentu
    # opcode - operacni kod instrukce
    def return_symb_attrib_type(self, arg, opcode):
        arg_type = arg.attrib.get('type', False)
        if arg_type == False:
            print("{}. instrukce, atribut instrukce 'type' nebyl nalezen", file=sys.stderr)
            sys.exit(XML_ERR)
        if arg_type not in{'var', 'bool', 'int', 'string'}:
            print("{}. instrukce, argument {} instrukce '{}' nema typ zadneho symbolu."
                .format(self.ins_order, arg.tag, opcode), file=sys.stderr)
            sys.exit(XML_ERR)
        else:
            return arg_type

    # zkontroluje, jestli je dany argument instrukce symbol
    # pokud ano, zkontroluje, jestli jsou konstanty spravneho typu
    # opcode - operacni kod instrukce
    # arg - element argumentu
    # exp_type - ocekavany typ
    def check_symbol(self, opcode, arg, exp_type):
        self.check_arg(opcode, arg)
        arg_type = self.return_symb_attrib_type(arg, opcode)

        err = 0
        if arg_type == 'var':
            if not self.is_specif_type(self.var_reg, arg):
                err = 1
        elif arg_type == 'int':
            if not self.is_specif_type(self.int_reg, arg):
                err = 1            
        elif arg_type == 'string':
            if not self.is_specif_type(self.string_reg, arg):
                err = 1
        elif arg_type == 'label':
            if not self.is_specif_type(self.label_reg, arg):
                err = 1
        elif arg_type == 'bool':
            if arg.text not in {'true', 'false'}:
                err = 1;

        if err == 1:
            print("{}. instrukce, argument {} instrukce {} neni symbol."
                .format(self.ins_order, arg.tag, opcode), file=sys.stderr)
            sys.exit(SEM_ERR)

        if arg_type != 'var' and exp_type != 'any':
            if arg_type != exp_type:
                print("{}. instrukce, argument {} instrukce {} ma spatny typ konstanty."
                    .format(self.ins_order, arg.tag, opcode), file=sys.stderr)
                sys.exit(SEM_ERR)

