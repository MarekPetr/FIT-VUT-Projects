#ifndef PROTOCOL
#define PROTOCOL

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#define PAYLOAD_SIZE 1024
#define ERR 1

int send_msg(int socket, char* msg, int code);
int recv_msg(int socket, char* buff, int* code);

typedef enum 
{
    FLAG,
    FLAG_OK,
    LOGIN,
    LOGIN_OK,
    DATA,
    DATA_OK,
    DATA_EMPTY,
    DATA_EMPTY_OK,
    MSG_CNT,
    MSG_CNT_OK,
    FAILURE,
} message_type;

#endif