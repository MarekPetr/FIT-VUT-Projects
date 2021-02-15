# Implementace prekladace imperativniho jazyka IFJ17
# Petr Marek,       login: xmarek66
# Jakub Stefanisin, login: xstefa22
# Petr Knetl,       login: xknetl00
#-Wall -Wextra -Werror -pedantic
CC=gcc -Wall -Wextra -Werror -pedantic
CFLAGS= -std=gnu99
OBJ=main.o garbage.o lex.o strings.o errors.o parser.o expresion.o code_gen_expres.c symbol.o symtable.o

all: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -lm -o proj

clean:
	rm  *.o proj

