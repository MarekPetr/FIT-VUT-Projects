#!/usr/bin/env python3
# coding=utf-8
import re
import sys

SEM_ERR = 52
FRAME_ERR = 55

# Trida starajici se o ramce intepretu
class Frames:
    def __init__(self, prog_cntr):
        self.stack_frame = []
        self.global_frame = []        
        self.temp_frame = None
        self.prog_cntr = prog_cntr

    # synchronizuje hodnotu programoveho citace s tou v interpretu
    # prog_cntr - programovy citac interpretu
    def update_prog_cntr(self, prog_cntr):
        self.prog_cntr = prog_cntr

    # vytvori docasny ramec
    def create_frame(self):
        self.temp_frame = []

    # vypise chybove hlaseni neexistence ramce a skonci s chybou
    # name - jmeno ramce
    def frame_err(self, name):
        print("{}. instrukce, {} neexistuje"
                .format(self.prog_cntr+1, name),file=sys.stderr)
        sys.exit(FRAME_ERR)

    # definuje promennou local_frame
    # - vraci hodnotu z vrcholu zasobniku ramcu
    @property
    def local_frame(self):
        if len(self.stack_frame) > 0:
            return self.stack_frame[-1]
        else:
            self.frame_err('lokalni ramec')
        
    # z nazvu promenne zjisti ramec
    # var - nazev promenne
    def get_frame(self, var):
        if var.startswith('GF@'):
            return self.global_frame
        elif var.startswith('TF@'):
            if self.temp_frame is None:
                self.frame_err('docasny ramec')
            return self.temp_frame
        elif var.startswith('LF@'):
            return self.local_frame

    # presune docasny ramec na zasobnik ramcu
    # - stane se z nej lokalni
    def push_frame(self):
        if self.temp_frame is None:
            self.frame_err('docasny ramec')
        self.stack_frame.append(self.temp_frame)
        self.temp_frame = None

    # vyjme lokalni ramec z vrcholu zasobniku ramcu
    # stane se z nej docasny ramec
    def pop_frame(self):
        if len(self.stack_frame) > 0:
            self.temp_frame = self.stack_frame.pop()
        else:
            self.frame_err('lokalni ramec')
            
    ###################################################
        # vybere ramec pro vraceni instrukci BREAK
    # narozdil od get_frame nikdy neskonci s chybou
    # name - jmeno ramce
    def choose_frame(self, name):
        if name == 'GF':
            return self.global_frame
        elif name == 'LF':
            if len(self.stack_frame) > 0:
                return self.local_frame
            else:
                return None
            
        elif name == 'TF':
            return self.temp_frame
        else:
            return None

    # vraci ramec pro instrukci BREAK
    # v pripade neexistujiciho ramce vraci retezec "Nedefinovany"
    # name - jmeno ramce
    def return_frame(self, name):
        frame = self.choose_frame(name)
        if frame is None:
            return ("Nedefinovany")
        else:
            return frame