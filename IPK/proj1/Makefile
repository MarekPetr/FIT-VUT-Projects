CFLAGS         = -g -std=gnu99 -Wall -Wextra -Werror -pedantic -lpthread
#-lrt
CC              = gcc

all: server client

server:
	$(CC) server.c protocol.c $(CFLAGS) -o ipk-server

client:
	$(CC) client.c protocol.c $(CFLAGS) -o ipk-client


clean: 
	rm ipk-server ipk-client
