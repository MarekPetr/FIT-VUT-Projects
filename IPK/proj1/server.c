#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#include "protocol.h"

// semafor, potrebny pro synchronizace soucasne bezicich procesu serveru
sem_t *mutex;

// Server musi spravne ukoncit sve detske procesy
void sigchld_handler()
{
    int saved_errno = errno; // waitpid() muze errno prepsat
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// server skonci na signal SIGINT
void sigint_handler()
{
    sem_close(mutex);
    sem_unlink("/xmarek66_mutex");
    printf("\n");
    exit(0);
}

// server odesle chybu a ukonci proces obsluhujici klienta
void server_error(int socket, char* msg)
{
    fprintf(stderr, "%s\n", msg);
    send_msg(socket, "", FAILURE);
    close(socket);
    exit(0);
}

void err_exit(int socket)
{
    close(socket);
    exit(0);
}

// zjisti, jestli je retezec pre prefixem retezce str
// zdroj:
// https://stackoverflow.com/questions/4770985/how-to-check-if-a-string-starts-with-another-string-in-c
bool starts_with(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre),
		   lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

// načte požadovaná data ze souboru a po jednom řádku je odešle klientovi
int send_data(int socket, char* flag, char* login)
{
	FILE *file;
	if ((file = fopen("/etc/passwd", "r")) == NULL) {
        char* err_msg = "Server can not open file.";
        send_msg(socket, err_msg, FAILURE);
		fprintf(stderr, "%s\n", err_msg);
		return ERR;
	}
	char line[200];
	char* token = '\0';

    unsigned size = PAYLOAD_SIZE;
    unsigned curr_size = 0;

    char* string =  (char *) malloc (size);
    if (string == NULL)
    {        
        fprintf(stderr, "Err malloc\n");
        send_msg(socket, "Server can not allocate enough memory", FAILURE);
        free(string);
        return ERR;
    }
    string[0] = '\0';

    while (fgets(line, 200, file) != NULL) {
        if (line[0] == '\n') continue;

        // token je prvni cast radku pred ':', tedy login
        token = strtok(line,":");        

        if (strcmp(flag, "l") == 0) {        	
            // pokud neni misto loginu # (znaci prazdny login),
            // je nutne vybirat loginy podle prefixu
            if (strcmp(login, "#") != 0) {
                if (starts_with(login, token)) {
                	curr_size += strlen(token)+1;
                	if (curr_size >= size) {
			        	size += PAYLOAD_SIZE;
			        	string = (char *) realloc(string, size);
	       			}
                    strcat(string, token);
                    strcat(string, "\n");
                }
            } else { // jinak se appenduji vsechny loginy
            	curr_size += strlen(token)+1;
            	if (curr_size >= size) {
		        	size += PAYLOAD_SIZE;
		        	string = (char *) realloc(string, size);
       			}
                strcat(string, token);
                strcat(string, "\n");                
            }
            
        } else { // -n nebo -f argument
            if ((strcmp(token, login)) == 0) {
                // zjisti jmeno osoby a dalsi info o ni
                int order = 0;
                if (strcmp(flag, "n") == 0) order = 3;
                else order = 4; // pro argument -f - domaci adresar

                // precte ze souboru dalsi pozadovany token
                for (int i = 0; i < order; ++i) strtok(NULL,":");
                strcpy(string, strtok(NULL,":"));
                strcat(string,"\n");
                break;              
            }
        }
    }
    fclose(file);

    if (strcmp(string, "") == 0)
    {
        char* err_data = "Server has not found any matching data.";
        fprintf(stderr, "%s\n", err_data);
        if (send_msg(socket, err_data, DATA_EMPTY)) {
            free(string);
            return 0;
        }
    } else {
        // spocita pocet radku dat, ktere bude nutne odeslat
        int len = strlen(string);
        int ln = 0;
        for (int i = 0; i < len; ++i)
        {
            if (string[i] == '\n') ln++;            
        }
        sprintf(line, "%d", ln);
        if (send_msg(socket, line, MSG_CNT)) { // odesle pocet zprav s daty klientovi
            free(string);
            return ERR;
        }

        // zjisti, jestli zprava dosla klientovi
        char buff[PAYLOAD_SIZE];
        int code = 0;
        if (recv_msg(socket, buff, &code)) {
            free(string);
            return ERR;
        }
        if (code != MSG_CNT_OK)
        {
            fprintf(stderr, "%s\n", buff);
            free(string);
            return ERR;
        }

        bzero(line, 200);

        // posle data po radcich klientovi
        token = strtok(string,"\n");
        for (int i = 0; i < ln; ++i)
        {
            if (send_msg(socket, token, DATA)) {
                free(string);
                return ERR;
            }
            token = strtok(NULL,"\n");
        }

        // obdrzi DATA_OK
        if (recv_msg(socket, buff, &code)) {
            free(string);
            return ERR;
        }
        if (code != DATA_OK)
        {
            fprintf(stderr, "Client has not received data\n");
            free(string);
            return ERR;
        }
    }
    free(string);

	return 0;
}


int main (int argc, const char * argv[]) {

    signal(SIGINT, sigint_handler);

	int rc;
	int welcome_socket;
	struct sockaddr_in sa;
	struct sockaddr_in sa_client;
	char str[INET_ADDRSTRLEN];
    int port_number;
    int comm_socket;

    struct sigaction sig;

    char buff[PAYLOAD_SIZE];
    char flag[2];
    flag[2] = '\0';
    char login[100];
    login[100] = '\0';
    
    if (argc != 3){
        fprintf(stderr, "Wrong arg count.\n");
        return ERR;
    } else {
        if (strcmp(argv[1], "-p") == 0){
            for (unsigned int i = 0; i < strlen(argv[2]); i++)
                if (!isdigit(argv[2][i])) {
                    fprintf(stderr, "Port has to be a number.\n");
                    return ERR;
                }
            port_number = atoi(argv[2]);
            if (port_number <= 0 || port_number > 65535) {
                fprintf(stderr, "Port is out of range\n");
                return ERR;
            }
        } else {
            fprintf(stderr, "Wrong second argument.\n");
            return ERR; 
        }
    }

    socklen_t sa_client_len=sizeof(sa_client);
    // Vytvoreni soketu
	if ((welcome_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("ERROR: socket");
		exit(EXIT_FAILURE);
	}

    // nastavi socket jako znovupouzitelny
	int yes = 1;
    if (setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return ERR;
    }
	
	memset(&sa,0,sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(port_number);        
    
    // priradi socketu spravnou adresu
	if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) < 0)
	{
		perror("ERROR: bind");
		exit(EXIT_FAILURE);		
	}
    // Oznaci socket jako pasivni, ten bude vyckavat na pripojeni prichoziho spojeni
    // 10 je pocet spojeni, ktere mohou v jednu chvili cekat na pripojeni
	if ((listen(welcome_socket, 10)) < 0)
	{
		perror("ERROR: listen");
		exit(EXIT_FAILURE);
	}

    // nastavi obsluhu ukoncovani detskych procesu 
    sig.sa_handler = sigchld_handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sig, NULL) == -1) {
        perror("sigaction");
        return ERR;
    }    

    // otevre semafor pro synchronizaci procesu k obslouzeni vice klientu
    if ((mutex = sem_open("/xmarek66_mutex", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
        fprintf(stderr, "Can not create mutex\n");
        return ERR;
    }

    // hlavni cyklus k prijimani a obslouzeni klientu
	while(1)
	{
		comm_socket = accept(welcome_socket, (struct sockaddr*)&sa_client, &sa_client_len);		
		if (comm_socket == -1)
        {
            perror("accept");
            continue;
        }

		inet_ntop(AF_INET, &sa_client.sin_addr, str, sizeof(str));

        // fork je treba pro vyrizovani vice klientu zaroven
        if (!fork()) {
            close(welcome_socket);

            int code;

            // obdrzi parametr (l/f/n)
            if (recv_msg(comm_socket, buff, &code)) err_exit(comm_socket);
            if (code != FLAG) {
                server_error(comm_socket, "Server has not received flag");                
            } else {
                strncpy(flag, buff, 2);
                if (send_msg(comm_socket, "", FLAG_OK)) err_exit(comm_socket);
            }            
            
            // obdrzi LOGIN
            if (recv_msg(comm_socket, buff, &code)) err_exit(comm_socket);
            if (code != LOGIN) {
                server_error(comm_socket, "Server has not received login");
            } else {
                strcpy(login, buff);
                if (send_msg(comm_socket, "", LOGIN_OK)) err_exit(comm_socket);
            }

            // posle DATA
            sem_wait(mutex);
                send_data(comm_socket, flag, login);
            sem_post(mutex);

            close(comm_socket);
            exit(0);
        }
		close(comm_socket);
	}	
}
