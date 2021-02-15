#ifndef PROTOCOL
#define PROTOCOL

#define Type_A 1       // Ipv4 address              //checked
#define Type_AAAA 28   // Ipv6 address
#define Type_NS 2      // Nameserver                //checked
#define Type_CNAME 5   // canonical name
#define Type_PTR 12    // domain name pointer
#define buff_size 512
#define DNS_port 53
#define ERR 1
#define ERR_ARG 2
#define name_size 256
#define INTERNET 1
#define CONT 6
#define HELP 3

//DNS header structure
typedef struct
{
    unsigned short id; // identification number
 
    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag
 
    unsigned char rcode :4; // response code
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available
 
    unsigned short qdcount; // number of question entries
    unsigned short ancount; // number of answer entries
    unsigned short nscount; // number of name server entries
    unsigned short arcount; // number of additional resource entries
}Header;
 
//Constant sized fields of query structure
typedef struct {
    unsigned short qtype;
    unsigned short qclass;
}Quest_info;
 
//Constant sized fields of the resource record structure
#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned short rclass;
    signed int ttl;
    unsigned short rdlength;
}R_metadata;
#pragma pack(pop)
 
//Pointers to resource record contents
typedef struct {
    char name[name_size];
    R_metadata *resource;
    char *rdata;
}Res_record;

typedef struct
{
    char dns_server[name_size];
    char hostname[name_size];
    int qtype;
    int iter;  
    int timeout;
}Arguments;

// Structure to save answers for iterative query
typedef struct
{   
    char answer[name_size]; //ANSWER SECTION answer or ADDITIONAL SECTION first answer
    char name[name_size];
    int type; // 0 for no answer, else same as qtype
}Answers;

int parse_args(Arguments *args, int argc ,char *argv[]);
int reverse_ipv4(char* addr);
int reverse_ipv6(char* addr);
int get_family(char* addr);
int  dns_gethostbyname(Arguments args, Answers *ans);
void host_to_qname(char* qname, char* host);
void read_domain_name(char* name, unsigned char* read_ptr, unsigned char* buffer, int* name_len);

#endif