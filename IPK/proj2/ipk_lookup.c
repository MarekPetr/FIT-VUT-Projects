#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>

#include "protocol.h"

int main(int argc , char *argv[])
{
    int err = 0;

    Arguments args;
    args.iter = 0;
    args.timeout = 5;
    args.qtype = Type_A;
    
    char name[name_size];
    
    err = parse_args(&args, argc, argv);
    if (err == HELP) return 0;
    else if (err != 0) return err;

    strcpy(name, args.hostname); // save hostname asked by user    

    Answers ans;
    ans.type = 0;
    ans.answer[name_size] = '\0';

    if (args.iter) {
        int type = args.qtype;        
        // search for Root server NS
        ans.type = 0;
        strcpy(args.hostname,".");  
        args.qtype = Type_NS;
        err = dns_gethostbyname(args, &ans);
        if (err != 0)
            return err;
        
        // search for Root server IP
        ans.type = 0;
        strcpy(args.hostname, ans.answer);
        args.qtype = Type_A;
        err = dns_gethostbyname(args, &ans);
        if (err != 0)
            return err;
        args.qtype = type;        
        strcpy(args.hostname, name);   
        
        err = CONT;
        int i = 0;
        while (err == CONT)
        {
            ans.type = 0;
            strcpy(args.dns_server, ans.answer);
            strcpy(args.hostname, name);
            args.qtype = type;
            err = dns_gethostbyname(args, &ans);
            i++;
        }
        if (err) return 1;
    
    } else {
        err = dns_gethostbyname(args, &ans);
        if (err != 0)
            return err;
    }

    return err;
}

int dns_gethostbyname(Arguments args, Answers *ans)
{   
    char name[name_size];
    strcpy(name,args.hostname);

    if (args.qtype == Type_PTR) {
        int family = get_family(name);
        if (family == 4) {
            reverse_ipv4(name);
        } else if (family == 6) {
            reverse_ipv6(name);
        } else {
            fprintf(stderr, "Name is not an IP address\n");
            return ERR;
        }
    }

    unsigned char buff[buff_size];
    bzero(buff,buff_size);
    int err = 0;
    int comm_socket;
    struct sockaddr_in serv_addr;    
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(args.dns_server);
    serv_addr.sin_port = htons(DNS_port);

    comm_socket = socket(serv_addr.sin_family , SOCK_DGRAM , 0);
    if (comm_socket < 0) {
        fprintf(stderr, "Could not create socket\n");
        return ERR;
    }
    // set timeout for send and receive operations
    struct timeval timeout;      
    timeout.tv_sec = args.timeout;
    timeout.tv_usec = 0;
    if (setsockopt (comm_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) == -1){
        perror("setsockopt send");
        return ERR;
    }
    if (setsockopt (comm_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
        perror("setsockopt recv");
        return ERR;
    }

    Header *header;
    header = (Header*)&buff;

    header->id = (unsigned short) htons(getpid());
    header->qr = 0;                 // This is a query
    header->opcode = 0;             // query type
    header->aa = 0;                     // Authoritative Answer
    header->tc = 0;                 // TrunCation
    if (args.iter == 1) {
        header->rd = 0;             // Recursion Desired flag
    } else {
        header->rd = 1;
    }
    header->ra = 0;                 // Recursion Available flag
    header->z = 0;
    header->rcode = 0;
    header->qdcount = htons(1);     // 1 question
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;

    /*
     MESSAGE FORMAT
     Header and Question mandatory, others only in the answer to query
        +---------------------+
        |        Header       |
        +---------------------+
        |       Question      | the question for the name server = Quest_info + qname
        +---------------------+
        |        Answer       | RRs answering the question
        +---------------------+
        |      Authority      | RRs pointing toward an authority
        +---------------------+
        |      Additional     | RRs holding additional information
        +---------------------+
    */
    char* qname;
    qname =(char*)&buff[sizeof(Header)];
    host_to_qname(qname , name);
    size_t qname_size = (strlen((const char*)qname) + 1);
    size_t header_qname_size = sizeof(Header) + qname_size;
    size_t quest_msg_size = header_qname_size + sizeof(Quest_info);

    Quest_info *qinfo;
    qinfo =(Quest_info*)&buff[header_qname_size]; //fill it    
    qinfo->qtype = htons(args.qtype); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(INTERNET);
 

    err = sendto(comm_socket, buff, quest_msg_size, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(err == -1) {   
        close(comm_socket);
        perror("sendto failed");
        return ERR;
    }
     
    //Receive the answer
    bzero(buff,buff_size);
    int serv_addr_len = sizeof(serv_addr);
    err = recvfrom (comm_socket,buff, buff_size , 0, (struct sockaddr*)&serv_addr, (socklen_t*)&serv_addr_len);
    if(err == -1) {
        close(comm_socket);
        perror("recvfrom failed");
        return ERR;
    }
    err = 0;
 
    header = (Header*) buff;
    int ancount = ntohs(header->ancount); // number of answer entries
    int nscount = ntohs(header->nscount); // authoritative nameservers cnt
    int arcount = ntohs(header->arcount);
    
    char* err_msg;
    if (header->rcode != 0) {        
        if      (header->rcode == 1) err_msg = "Format error\n";
        else if (header->rcode == 2) err_msg = "Server failure\n";
        else if (header->rcode == 3) err_msg = "Name error\n";
        else if (header->rcode == 4) err_msg = "Not Implemented\n";
        else if (header->rcode == 5) err_msg = "Refused\n";
        else                         err_msg = "Unknown error code\n";        
        fprintf(stderr, "Response code error: %s", err_msg);
        close(comm_socket);
        return ERR;
    }

    if (header->tc == 1) {
        fprintf(stderr, "Answer is too long and was truncated\n");
        close(comm_socket);
        return ERR;
    }

    int stop=0;
    int i = 0;  
    int ret_code = ERR;
    if (args.iter == 1) ret_code = CONT;

    Res_record answers[256],name_servers[256], addit[256];

    // answer section is right after header and question sections
    unsigned char *read_ptr = &buff[quest_msg_size];

    // fill Res_record answers structure with data received
    for(i = 0; i < ancount; i++)
    {
        // read Res_record.name
        read_domain_name(answers[i].name, read_ptr, buff, &stop);
        read_ptr += stop; // offset read_ptr after 'name' -> to 'resource'
        
        // read Res_record.resource
        answers[i].resource = (R_metadata*)read_ptr; // resource starts on read_ptr
        int answer_type = ntohs(answers[i].resource->type);
        read_ptr += sizeof(R_metadata); // offset read_ptr after R_metadata -> to 'rdata'
        // if server answered at least once with right type, program returns 0, else 1
        if (answer_type == args.qtype) ret_code = 0;
        else ret_code = ERR;

        int alloc_size = 0;
        if (answer_type == Type_A) {
            alloc_size = ntohs(answers[i].resource->rdlength) + 1; // +1 for zero byte 
        } else {
            alloc_size = name_size;
        }
        answers[i].rdata = (char*) malloc(alloc_size);
        if (answers[i].rdata == NULL) {
            fprintf(stderr, "Malloc failed\n");
            for (int j = 0; j < i; j++)
            {
                free(answers[j].rdata);
            }
            close(comm_socket);
            return ERR;
        }  

        // read Res_record.rdata
        if (answer_type == Type_A || answer_type == Type_AAAA) { // it is IPv4 addr
            int data_len = ntohs(answers[i].resource->rdlength);
            for (int j = 0; j < data_len; j++)
            {
               answers[i].rdata[j] = read_ptr[j];
            }
            answers[i].rdata[data_len] = '\0';
            read_ptr += data_len;
        } else {
            read_domain_name(answers[i].rdata, read_ptr, buff, &stop);
            read_ptr += stop;
        }
    }

    // fill Res_record name server entries
    for (int i = 0; i < nscount; i++)
    {
        read_domain_name(name_servers[i].name, read_ptr, buff, &stop);
        read_ptr += stop;

        // resource starts on read_ptr
        name_servers[i].resource = (R_metadata*)read_ptr;
        read_ptr += sizeof(R_metadata);

        name_servers[i].rdata = (char*) malloc(name_size);
        if (name_servers[i].rdata == NULL) {
            fprintf(stderr, "Malloc failed\n");
            for (int j = 0; j < i; j++)
            {
                free(name_servers[i].rdata);
            }
            for (int j = 0; j < ancount; j++)
            {
                free(answers[i].rdata);
            }
            close(comm_socket);
            return ERR;
        }  
        read_domain_name(name_servers[i].rdata, read_ptr, buff, &stop);
        read_ptr += stop;
    }

    // read additional
    for(i = 0; i < arcount; i++)
    {
        // read Res_record.name
        read_domain_name(addit[i].name, read_ptr, buff, &stop);
        read_ptr += stop; // offset read_ptr after 'name' -> to 'resource'
        
        // read Res_record.resource
        addit[i].resource = (R_metadata*)read_ptr; // resource starts on read_ptr
        int answer_type = ntohs(addit[i].resource->type);
        read_ptr += sizeof(R_metadata); // offset read_ptr after R_metadata -> to 'rdata'
        // if server answered at least once with right type, program returns 0, else 1

        int alloc_size = 0;
        if (answer_type == Type_A) {
            alloc_size = ntohs(addit[i].resource->rdlength) + 1; // +1 for zero byte 
        } else {
            alloc_size = name_size;
        }
        addit[i].rdata = (char*) malloc(alloc_size);
        if (addit[i].rdata == NULL) {
            fprintf(stderr, "Malloc failed\n");
            for (int j = 0; j < ancount; j++)
            {
                free(answers[j].rdata);
            }
            for (int j = 0; j < nscount; j++)
            {
                free(name_servers[j].rdata);
            }
            for (int j = 0; j < i; j++)
            {
                free(addit[j].rdata);
            }
            close(comm_socket);
            return ERR;
        }
        // read Res_record.rdata
        if (answer_type == Type_A || answer_type == Type_AAAA) { // it is IPv4 addr
            int data_len = ntohs(addit[i].resource->rdlength);
            for (int j = 0; j < data_len; j++)
            {
               addit[i].rdata[j] = read_ptr[j];
            }
            addit[i].rdata[data_len] = '\0';
            read_ptr += data_len;
        } else {
            read_domain_name(addit[i].rdata, read_ptr, buff, &stop);
            read_ptr += stop;
        }
    }

    //print answers
    struct sockaddr_in addr4;
    for(i = 0; i < ancount; i++)
    {        
        int answer_type = ntohs(answers[i].resource->type);
        char *type_str; 
        if      (answer_type == Type_A)     type_str = "A";
        else if (answer_type == Type_AAAA)  type_str = "AAAA";
        else if (answer_type == Type_NS)    type_str = "NS";
        else if (answer_type == Type_CNAME) type_str = "CNAME";
        else if (answer_type == Type_PTR)   type_str = "PTR";

        if ((args.iter && i == 0) || !args.iter)
        {
            printf("%s. IN %s ", answers[i].name, type_str);
            if (answer_type == Type_A) {
                addr4.sin_addr.s_addr = *(long*)answers[i].rdata;
                printf("%s", inet_ntoa(addr4.sin_addr));

                // adds answer to array
                if (args.iter && !ans->type) {
                    strcpy(ans->answer,inet_ntoa(addr4.sin_addr));
                    strcpy(ans->name, answers[i].name);
                    ans->type = Type_A;
                }  
            } else if (answer_type == Type_CNAME || answer_type == Type_NS || 
                (answer_type == Type_PTR))
            {
                printf("%s.",answers[i].rdata);
                // adds name server to array
                if(args.iter && !ans->type) {
                    strcpy(ans->answer,answers[i].rdata);
                    strcpy(ans->name, answers[i].name);
                    ans->type = answer_type;
                }                  
            } else if (answer_type == Type_AAAA) {
                char address[INET6_ADDRSTRLEN];
                if (inet_ntop(AF_INET6, name, address, sizeof(address)) == NULL) {
                    perror("inet_ntop");
                    close(comm_socket);
                    return ERR;
                }
                printf("%s",address);
            }
            printf("\n");
        }            
        
    }

    if (args.iter == 1 && ancount == 0)  {
        //print first Authority nameserver
        for (int i = 0; i < nscount; i++)
        {
            int resource_type = ntohs(name_servers[i].resource->type);
            if (resource_type == Type_NS) {
                if (i == 0) {
                    // we are in recursive query, so we need more nameservers - can not exit with no answer
                    printf("%s. IN NS ",name_servers[i].name);
                    printf("%s.\n",name_servers[i].rdata);
                    ret_code = CONT;
                }

            }          
        }
        //print additional A type info
        for(i = 0; i < arcount; i++)
        {        
            int answer_type = ntohs(addit[i].resource->type);
            if (answer_type == Type_A) {
                if (i == 0) {
                    printf("%s. IN A ", addit[i].name);
                    addr4.sin_addr.s_addr = *(long*)addit[i].rdata;
                    printf("%s\n", inet_ntoa(addr4.sin_addr));
                }                

                // adds answer to array
                if (ans->type == 0) {
                    for (int j = 0; j < nscount; j++)
                    {
                        if (strcmp(name_servers[j].rdata, addit[i].name) == 0) {                            
                            strcpy(ans->answer,inet_ntoa(addr4.sin_addr));
                            ans->type = Type_A;
                            ret_code = CONT;
                            break;
                        }  
                    }                                      
                }            
            }
        }
    }
    

    for (int i = 0; i < ancount; i++)
    {
        free(answers[i].rdata);
    }

    for (int i = 0; i < nscount; i++)
    {
        free(name_servers[i].rdata);
    }

    for (int i = 0; i < arcount; i++)
    {
        free(addit[i].rdata);
    }

    close(comm_socket);
    return ret_code;
}

int parse_args(Arguments *args, int argc ,char *argv[])
{   
    int server_set = 0;
    int opt;
    int h = 0;
    int s = 0;
    int T = 0;
    int t = 0;
    int i = 0;

    while ((opt = getopt(argc, argv, "hs:T:t:i")) != -1) {
        switch(opt) {
            case 'h':
                if (h) {
                    fprintf(stderr, "h parameter can be used only once\n");
                    return ERR_ARG;
                }
                printf("NAPOVEDA\n");
                printf("---------------\n");
                printf("./ipk-lookup -s server [-T timeout] [-t type] [-i] name\n");
                printf("    h (help)      - volitelný parametr, při jeho zadání se vypíše nápověda a program se ukončí.\n");
                printf("    s (server)    - povinný parametr, DNS server (IPv4 adresa), na který se budou odesílat dotazy.\n");
                printf("    T (timeout)   - volitelný parametr, timeout (v sekundách) pro dotaz, výchozí hodnota 5 sekund.\n");
                printf("    t (type)      - volitelný parametr, typ dotazovaného záznamu: A (výchozí), AAAA, NS, PTR,CNAME.\n");
                printf("    i (iterative) - volitelný parametr, vynucení iterativního způsobu rezoluce, viz dále.\n");
                printf("    name          - překládané doménové jméno, v případě parametru -t PTR\n");
                printf("                    program na vstupu naopak očekává IPv4 nebo IPv6 adresu.\n");
                h = 1;
                break;
            case 's':
                if (s) {
                    fprintf(stderr, "s parameter can be used only once\n");
                    return ERR_ARG;
                }
                if (get_family(optarg) == 0) {
                    fprintf(stderr, "Server %s is not an IP address\n",optarg);
                    return ERR_ARG;
                }                    
                strcpy(args->dns_server, optarg);
                server_set = 1;
                s = 1;
                break;
            case 'T':
                if (T) {
                    fprintf(stderr, "T parameter can be used only once\n");
                    return ERR_ARG;
                }
                for (unsigned int i = 0; i < strlen(optarg); i++)
                {
                   if (!isdigit(optarg[i])) {
                        fprintf(stderr, "Timeout has to be a number\n");
                        return ERR_ARG;
                    } 
                }
                args->timeout = atoi(optarg);
                T = 1;
                break;
            case 't':
                if (t) {
                    fprintf(stderr, "t parameter can be used only once\n");
                    return ERR_ARG;
                }
                if (strcmp(optarg, "A")   == 0) {
                    args->qtype = Type_A;
                } else if (strcmp(optarg, "AAAA")  == 0) {
                    args->qtype = Type_AAAA;
                } else if (strcmp(optarg, "NS")  == 0) {
                    args->qtype = Type_NS;
                } else if (strcmp(optarg, "CNAME") == 0) {
                    args->qtype = Type_CNAME;
                } else if (strcmp(optarg, "PTR") == 0) {
                    args->qtype = Type_PTR;
                } else {
                    fprintf(stderr, "Wrong -t parameter (type)\n");
                    return ERR_ARG;
                }
                t = 1;        
                break;
            case 'i':
                if (i) {
                    fprintf(stderr, "i parameter can be used only once\n");
                    return ERR_ARG;
                }
                args->iter = 1;
                i = 1;
                break;
            default :
                fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n", argv[0]);
                return ERR_ARG;
        }
    }

    if (args->qtype == Type_PTR && args->iter == 1)
    {
        fprintf(stderr, "PTR iteratively is not supported yet\n");
        return ERR;
    }
    if (h) return HELP;

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        return ERR_ARG;
    }
    if(!server_set) {
        fprintf(stderr, "-s option is mandatory\n");
        return ERR_ARG;
    }
    strcpy(args->hostname, argv[optind]);
    

    return 0;
}

void read_domain_name(char* name, unsigned char* read_ptr, unsigned char* buffer, int* count)
{
    unsigned int p=0,jump=0,offset;  
 
    *count = 1;
    if (name == NULL)
    {
        fprintf(stderr, "read_domain_name, name is not allocated!\n");
    }
 
    name[0]='\0';
 
    // read names in 3www6seznam2cz0 format
    for (; *read_ptr != 0; read_ptr++)
    {
        if (*read_ptr<192) { // there are pure data
            name[p]=*read_ptr;
            p++;
        } else { // we need to read data where a pointer points
            jump = 1; // jumped to data
             //49152 = 11000000 00000000 = deleting two ones
            offset = *(read_ptr+1) + (*read_ptr)*256 - 49152;
            read_ptr = buffer + offset - 1;            
        }         
        if(jump==0) (*count)++;
    }
 
    name[p]='\0';
    if(jump==1) (*count)++;

    // converts 3www6seznam2cz0 to www.seznam.cz
    int name_len = strlen((char*)name);
    int i;
    for(i = 0; i < name_len; i++) 
    {
        p=name[i];
        for(unsigned int j = 0; j < p; j++) 
        {
            name[i] = name[i+1];
            i++;
        }
        name[i]='.';
    }
    name[i-1]='\0';
}
 
// convert www.google.com to 3www6google3com0
void host_to_qname(char* qname,char* host) 
{
    unsigned int to_dot = 0;

    if (host[strlen(host)-1] != '.') {
        strcat(host,".");
    }
    unsigned int host_len = strlen(host);
     
    for(unsigned int i = 0; i < host_len; i++) 
    {
        if(host[i]=='.') 
        {
            *qname++ = i-to_dot;
            while (to_dot<i)
            {
                *qname++=host[to_dot++];
            }
            to_dot++;
        }
    }
    *qname++='\0';
}

int reverse_ipv4(char* addr)
{
    int a, b, c, d, cnt;    
    sscanf(addr,"%d.%d.%d.%d", &a, &b,&c,&d);
    cnt = snprintf(addr, INET_ADDRSTRLEN, "%d.%d.%d.%d", d, c, b, a);
    if (cnt < 0 || cnt > INET_ADDRSTRLEN) {
        fprintf(stderr, "Address was not reversed succesfully\n");
        return ERR;
    } else {
        strcat(addr, ".in-addr.arpa");
    }
    return 0;
}

int reverse_ipv6(char* addr)
{
    char aux[8][4]; // eight groups of four hexadecimal digits
    char aux_2[8][4];
    //printf("%s\n",addr);

    // reversed address
    char reversed[name_size];
    
    // zero buffer
    for (int i = 0; i < name_size; i++)
    {
        reversed[i] = '0';
    }
    // //printf("len:%d\n",(int)strlen(addr));
    // copy address to zeroed buffer
    // addr for example: 2001:db8::567:89ab 
    for (unsigned int i = 0; i < strlen(addr); i++)
    {
        reversed[i] = addr[i];
    }
    reversed[name_size] = '\0';

    int p = 0; // pointer in source address addr
    int i = 0; // 8 groups of four bytes (0-7)
    int j = 0; // four bytes (0-3)
    int k = 0; // auxiliary counter for moving bytes in partial group (1-3 bytes)
    char last_char = '0';
    int shortened = 0;
    int groups = 0;
    int offset = 0;
    int to_move = 0;

    // zero auxiliary buffer
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            aux[i][j] = '0';
        }
    }
    // fills multibyte array 'aux' with address in 'reversed'
    // omits ':' and complete incomplete groups of four with zeroes
    // result for example:
    //  i:| 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7
    //     2001 0db8 0000 0567 89ab 0000 0000 0000
    //  j: 0123 0123 0123 0123 0123 0123 0123 0123 
    for (i = 0; i < 8; i++)
    {        
        // 'shortened' indicates first group of zeroes shortened to ::
        if (last_char == ':' && reversed[p] == ':') {
            shortened = i;
        }
        // fill group of four
        for (j = 0; j < 4 && reversed[p] != ':'; j++)
        {   
            aux[i][j] = reversed[p];
            p++; 
        }
        if (reversed[p] == ':') groups++;

        last_char = reversed[p];
        // if there are less than 4 digits, move them right 
        // and zero the rest from beginning
        p++;
        if (j < 4 && j > 0) {
            offset = 4 - j; // = 1
            for (k = j; k > 0; k--)
            {  
                aux[i][k] = aux[i][k-offset];
            }
            aux[i][k] = '0';
        }
    }
    // if there is group of zeroes indicated by '::' in addr,
    // move other groups in aux array which are after this group to end of whole address
    // and fill bytes in zero groups with zeroes
    // result for example: 2001 0db8 0000 0000 0000 0000 0567 89ab
    if (shortened) {
        offset = 7 - groups;
        to_move = groups - shortened;
        for (int i = 7; to_move > 0; i--)
        {
            for (int j = 0; j < 4; j++)
            {
                aux[i][j] = aux[i-offset][j];
                aux[i-offset][j] = '0';
            }
            to_move--;
        }        
    }
    // copy array to another one, so we can reverse it
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            aux_2[i][j] = aux[i][j];
        }
    }
    // reverse the array
    for (i = 0; i < 8; i++)
    {
        // copy first bit to last and second to next-to-last, etc
        // result for example: ba98 7650 0000 0000 0000 0000 8bd0 1002
        for (j = 0; j < 4; j++)
        {
            aux[i][j] = aux_2[7-i][3-j];
        }
    }
    // copy two dimensional array to one dimensional target array 
    // then copy this new_addr to addr (changes source address)
    // result for example: b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2
    p = 0;
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {            
            reversed[p++] = aux[i][j];
            reversed[p++] = '.';
        }
    }    
    reversed[64] = '\0'; 
    strcat(reversed, "ip6.arpa");
    // result for example:
    // b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa
    strncpy(addr, reversed, name_size);
    return 0;
}

int get_family(char* addr)
{
    char buffer[name_size];
    if (inet_pton(AF_INET, addr, buffer)) {
        return 4;
    } else if (inet_pton(AF_INET6, addr, buffer)) {
        return 6;
    }
   return 0;
}