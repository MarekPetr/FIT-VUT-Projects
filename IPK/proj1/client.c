#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include "protocol.h"

typedef struct {
    char* host;
    int port;
    char* login;
    char* flag;
} arguments;

arguments parseArgs(int argc, char *argv[]) {

    arguments args;

    if (argc < 6 || argc > 7) {
        fprintf(stderr, "Wrong args count\n");
        exit(ERR);
    }

    if (strcmp(argv[1], "-h") == 0) {
        args.host = argv[2];
        if (strcmp(argv[3], "-p") == 0) {
            for (unsigned int i = 0; i < strlen(argv[4]); i++)
                if (!isdigit(argv[4][i])) {
                    fprintf(stderr, "Port has to be a number.\n");
                    exit(ERR);
                }
            args.port  = atoi(argv[4]);
            if (args.port <= 0 || args.port > 65535) {
                fprintf(stderr, "Client: Port is out of range\n");
                exit(ERR);
            }
        }
    }

    if (strcmp(argv[5],"-l") != 0) {
        if (argc != 7) {
            fprintf(stderr, "Wrong args count\n");
            exit(ERR);
        }
        if (strcmp(argv[5], "-f") == 0) {
            args.flag = "f";
        } else if (strcmp(argv[5], "-n") == 0) {
            args.flag = "n";        
        } else {
            fprintf(stderr, "Wrong argument option.\n");
            exit(ERR);
        }
    } else {
        args.flag = "l";
    }

    if (argv[6] != NULL)
        args.login = argv[6];
    else 
        args.login = "#";

    return args;
}

int main (int argc, char * argv[]) {
	int comm_socket, port_number;
    const char *server_hostname;
    struct hostent *server;
    struct sockaddr_in server_address;
    char buff[PAYLOAD_SIZE];

    arguments args;
    args = parseArgs(argc, argv);

    server_hostname = args.host;
    port_number = args.port;
    
    // ziskani adresy serveru pomoci DNS    
    if ((server = gethostbyname(server_hostname)) == NULL) {
        fprintf(stderr,"ERROR: no such host as %s\n", server_hostname);
        exit(ERR);
    }    

    // nalezeni IP adresy serveru a inicializace struktury server_address
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port_number);
    
    // Vytvoreni soketu
	if ((comm_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("ERROR: socket");
		exit(ERR);
	}
    
    // pripojeni se na socket
    if (connect(comm_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) == -1)
    {
		perror("ERROR: connect");
		exit(ERR);        
    }
    
    int code;
    //*************************************************************************
    // posle parametr (l/n/f) - FLAG
    if(send_msg(comm_socket, args.flag, FLAG)) {
        close(comm_socket);
        return ERR;
    }
    // obdrzi FLAG_OK
    if(recv_msg(comm_socket, buff, &code)) {
        close(comm_socket);
        return ERR;
    }
    if (code != FLAG_OK) {
        fprintf(stderr, "Server has not received flag\n");
        return ERR;
    }
    //*************************************************************************
    // posle login
    if(send_msg(comm_socket, args.login, LOGIN)) {
        close(comm_socket);
        return ERR;
    }
    // obdrzi LOGIN_OK
    if(recv_msg(comm_socket, buff, &code)) {
        close(comm_socket);
        return ERR;
    }
    if (code != LOGIN_OK) {
        fprintf(stderr, "Server has not received login\n");
        return ERR;
    }
    //*************************************************************************
    // obdrzi pocet zprav s daty, ktere bude nutne precist
    int cnt = 0;
    if(recv_msg(comm_socket, buff, &code)) {
        close(comm_socket);
        return ERR;
    }    
    if (code == DATA_EMPTY || code == FAILURE)
    { 
        fprintf(stderr, "%s\n", buff);
        close(comm_socket);
        return 0;
    } else if (code != MSG_CNT) {
        char* err_msg = "Client has not received info about data length.";
        send_msg(comm_socket, err_msg, FAILURE);
        fprintf(stderr, "%s\n", err_msg);
        close(comm_socket);
        return ERR;
    }
    // odesle potvrzeni, ze informace o velikosti dat prisla
    if(send_msg(comm_socket, "", MSG_CNT_OK)) {        
        close(comm_socket);
        return ERR;
    }   
    sscanf(buff, "%d", &cnt);
    //*************************************************************************
    // prijme a vytiskne data na STDOUT
    for (int i = 0; i < cnt; ++i)
    {
        if(recv_msg(comm_socket, buff, &code)) {
            close(comm_socket);
            return ERR;
        }
        printf("%s\n", buff);
    }
    //*************************************************************************
    // odesle potvrzeni o prijatych datech
    if(send_msg(comm_socket, "", DATA_OK)) {
        close(comm_socket);
        return ERR;
    }
    //*************************************************************************
    close(comm_socket);
    return 0;
}
