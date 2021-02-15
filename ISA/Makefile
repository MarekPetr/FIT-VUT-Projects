CC= g++ -Wall -Wextra -Werror -pedantic
CFLAGS= -std=c++11
OBJ= main.o handlers.o dns_parser.o slog.o sniffer.o base64.o

all: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -lm -o dns_export  -L/usr/lib64 -lpcap -Wunknown-pragmas

clean:
	rm  *.o dns_export