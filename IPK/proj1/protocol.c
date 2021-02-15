#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "protocol.h"

// zdroj Send, Recv 
// https://stackoverflow.com/questions/39236089/socket-sending-a-big-string-in-c
int Send(int sockfd, char *buf, int len) {
    int sent_total = 0;
    for (int sent_now = 0; sent_total != len; sent_total += sent_now) {
        sent_now = send(sockfd, buf + sent_total, len - sent_total, 0);
        if (sent_now == -1) {
            fprintf(stderr, "Send failed to send all data\n");
            return -1;
        }
    }
    return sent_total;
}

int Recv(int sockfd, char *buf, int len) {
  int recv_total = 0;
    for (int recv_now = 0; recv_total != len; recv_total += recv_now) {
        recv_now = recv(sockfd, buf + recv_total, len - recv_total, 0);
        if (recv_now == -1) {
            fprintf(stderr, "Recv failed to receive all data\n");
            return -1;
        }
    }
    return recv_total;
}

// vraci pocet bytu, ktere se neodeslali - vcetne prvnich 4 pro delku zpravy
// uspesne konci s nulou
int send_msg(int socket, char* msg, int code)
{
    char buf[PAYLOAD_SIZE];
    //***************************************************
    // Prvnich 4 byty jsou delka zpravy ve treti casti
    int total = 4;
    int data_bytes = strlen(msg);
    if (data_bytes < 0 || data_bytes > PAYLOAD_SIZE-1) {
        fprintf(stderr, "Data are too long to send\n");
        return -1;
    }
    sprintf(buf, "%04d", data_bytes);
    int sent = Send(socket, buf, total);
    if (sent == -1) return -1;
    //***************************************************
    // Dalsi 3 byty je kodove oznaceni zpravy
    bzero(buf, PAYLOAD_SIZE);
    total = 3;
    sprintf(buf, "%03d", code); 
    sent = Send(socket, buf, total);
    if (sent == -1) return -1;
    //***************************************************
    // samotna zprava
    total = strlen(msg);
    sent = Send(socket, msg, total);
    if (sent == -1) return -1;

    return 0;
}


// vraci pocet bytu zbyvajicich k precteni
// uspesne konci s nulou
int recv_msg(int socket, char* buff, int* code)
{
    char len_buf[4];
    char code_buf[3];
    bzero(len_buf, 4);
    bzero(code_buf, 3);
    int data_left = 0;

    //***************************************************
    // prvni cast - 4 byty pro delku zpravy
    bzero(buff, PAYLOAD_SIZE);    
    int left = 4;
    int received = 0;

    received = Recv(socket, len_buf, left);
    if (received == -1) return -1;
    len_buf[4] = '\0';
    sscanf(len_buf, "%4d", &data_left);
    // kontrola, jestli se data vlezou do buffru
    if (data_left < 0 || data_left > PAYLOAD_SIZE-1)
    {
        fprintf(stderr, "Data are too long to recv\n");
        return -1;
    }
    //***************************************************
    // druha cast - 3 byty pro kodove oznaceni zpravy 
    left = 3;
    received = Recv(socket, code_buf, left);
    if (received == -1) return -1;
    code_buf[3] = '\0';
    sscanf(code_buf, "%3d", code);    
    //***************************************************
    // posledni cast - samotna zprava
    received = Recv(socket, buff, data_left);
    buff[PAYLOAD_SIZE] = '\0';
    if (received == -1) return -1;

    return 0;
}