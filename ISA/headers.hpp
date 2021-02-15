#ifndef DNS_PROTOCOL
#define DNS_PROTOCOL

#include <pcap.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <netdb.h>
#include <sstream>
#include <stdint.h>

#define Type_A 1       // Ipv4 address
#define Type_AAAA 28   // Ipv6 address
#define Type_NS 2      // Nameserver
#define Type_CNAME 5   // canonical name
#define Type_PTR 12    // domain name pointer
#define Type_SOA 6
#define Type_TXT 16
#define Type_MX 15
#define Type_DNSKEY 48
#define Type_NSEC 47
#define Type_DS 43
#define Type_RRSIG 46
#define UNKNOWN_TYPE -1

#define DNS_port 53
#define ERR 1
#define ERR_ARG 2
#define name_size 256
#define INTERNET 1

#define FILE 1
#define DEVICE 2

#define TCP 1
#define UDP 2

// default snap length (maximum bytes per packet to capture)
#define SNAP_LEN 1518

using namespace std;
    
// Res_record
/*                                              1  1  1  1  1  1
                  0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                |                                               |
                /                                               /
                /                      NAME                     /
                |                                               |
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
             -- |                      TYPE                     |
             |  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
             |- |                     CLASS                     |
             |  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
R_metadata --|- |                      TTL                      |
             |  |                                               |
             |  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
             -- |                   RDLENGTH                    |
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
                /                     RDATA                     /
                /                                               /
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/
//DNS UDP header structure
typedef struct
{   
    // TCP adds 'uint16_t length' // processed in sniffer
    uint16_t id; // identification number
 
    uint8_t rd :1; // recursion desired
    uint8_t tc :1; // truncated message
    uint8_t aa :1; // authoritive answer
    uint8_t opcode :4; // purpose of message
    uint8_t qr :1; // query(0) / response flag(1)
 
    uint8_t rcode :4; // response code
    uint8_t z :1; // its z! reserved
    uint8_t ra :1; // recursion available
 
    uint16_t qdcount; // number of question entries
    uint16_t ancount; // number of answer entries
    uint16_t nscount; // number of name server entries
    uint16_t arcount; // number of additional resource entries
}Header;

// TCP header
typedef uint32_t tcp_seq;

struct tcphdr {
    u_short th_sport;       // source port
    u_short th_dport;       // destination port
    tcp_seq th_seq;         // sequence number
    tcp_seq th_ack;         // acknowledgement number
#if BYTE_ORDER == LITTLE_ENDIAN
    u_char  th_x2:4,        // (unused)
        th_off:4;           // data offset
#endif
#if BYTE_ORDER == BIG_ENDIAN
    u_char  th_off:4,       // data offset
        th_x2:4;            // (unused)
#endif
    u_char  th_flags;
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80
#define TH_FLAGS    (TH_FIN|TH_SYN|TH_RST|TH_PUSH|TH_ACK|TH_URG|TH_ECE|TH_CWR)
#define PRINT_TH_FLAGS  "\20\1FIN\2SYN\3RST\4PUSH\5ACK\6URG\7ECE\10CWR"

    u_short th_win;         // window
    u_short th_sum;         // checksum
    u_short th_urp;         // urgent pointer
};
 
//Constant sized fields of query structure
typedef struct {
    uint16_t qtype;
    uint16_t qclass;
}Querie_info;
 
//Constant sized fields of the resource record structure
#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint16_t rclass;
    int32_t ttl;
    uint16_t rdlength;
}R_metadata;
 
//Pointers to resource record contents
typedef struct {
    char name[name_size];
    R_metadata *resource;
    char *rdata;
}Res_record;

// constant sized field of SOA record
typedef struct {
    uint32_t serial;
    uint32_t refresh;
    int32_t retry;
    int32_t expire;
    uint32_t minimum;
}SOA;

// constant sized field of DNSKEY record
typedef struct {
    uint16_t flags;
    uint8_t protocol;
    uint8_t algorithm;
    // Public key - not constant sized
}DNSKEY;

typedef struct {
    uint16_t key_tag; // 16 bits = 2 octets
    uint8_t algorithm;
    uint8_t digest_type;
    // Digest- not constant sized
}DS;

typedef struct {
    uint16_t type_covered;
    uint8_t algorithm;
    uint8_t labels;
    uint32_t original_ttl;
    uint32_t sign_expir;
    uint32_t sign_incept;
    uint16_t key_tag;
    // signer_name, signature
}RRSIG;
#pragma pack(pop)

// arguments
typedef struct
{   
    char pcap_file[name_size];
    char dns_server[name_size];
    char device[name_size];
    int run_flag; // 1 for pcap_file, 2 for device sniffing
    bool server_set;
    struct sockaddr_in serv_addr;
    struct addrinfo *p;
    struct addrinfo *servinfo;
    int comm_socket;
    int timeout;
}Arguments;

// Structure to save answers for iterative query
typedef struct
{   
    char answer[name_size]; //ANSWER SECTION answer or ADDITIONAL SECTION first answer
    char name[name_size];
    int type; // 0 for no answer, else same as qtype
    string result;
}Answer;

extern string records_str; // Global variable with all stats
extern pcap_t *handle;     // global packet capture handle
extern Arguments args;     // global arguments

// gets IP address or hostname of this host
string get_host();

/*
* Gets socket to server and saves it 
* in Arguments structure for future use.
*/
int get_socket();

/*
* Sends message msg to connected syslog server.
* Socket has to be provided in args.comm_socket.
*/
int sendto_syslog_server(string msg);

// cut string msg to lines and send them one by one to syslog server
int send_syslog_line(string msg);

/* 
* Parses packet to find DNS resource record data.
* Returns first answer in format string.
*/
string parse_dns_packet(u_char* data);

/* 
* Handles signals.
* On SIGUSR1 prints statistics.
*/
void sig_handler(int sig);

// handles alarm - sends statistics after timeout has elapsed
void alarm_handler(int sig);

/*
* Sets signal handlers, alarm timeout.
* Triggers alarm_handler on timeout.
*/
void set_sig_handlers(int timeout);

// calculates statistics and print them to stdout
void print_statistics();

// captures packets on interface or from file
int sniff();

/*
* Parses records_str global string and computes statistics.
* Appends these statistics to string returned by these function.
*/
string get_stats();

/* 
* Converts string src to char* dest.
* Cest has to be allocated by caller function.
*/
void string_to_char(char* dest, string src);

// converts unsigned char* to string
string char_to_string(unsigned char* src);

#endif