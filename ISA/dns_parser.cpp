#include "headers.hpp"
#include "base64.h"


using namespace std;

/* 
* Reads domain name - deals with pointers as addresses.
* Returns string of given name.
*/
string read_domain_name(char* name, unsigned char* read_ptr, unsigned char* buffer, int* count)
{   
    string domain;
    unsigned int p=0,jump=0,offset;
 
    *count = 1;
    if (name == NULL)
    {
        fprintf(stderr, "read_domain_name, name is not allocated!\n");
        return "";
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
            /* 49152 = 11000000 00000000 = deleting two ones
               This allows a pointer to be distinguished from label*/
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

    domain.append(name);
    return domain;
}

// returns type of DNS record
string get_qtype_string (int qtype, int* err) {
    *err = 0;
    string str;
    if      (qtype == 1)     str = "A";
    else if (qtype == 28)    str = "AAAA";
    else if (qtype == 18)    str = "AFSDB ";
    else if (qtype == 42)    str = "APL";
    else if (qtype == 257)   str = "CAA";
    else if (qtype == 60)    str = "CDNSKEY";
    else if (qtype == 59)    str = "CDS ";
    else if (qtype == 37)    str = "CERT";
    else if (qtype == 5)     str = "CNAME";
    else if (qtype == 49)    str = "DHCID";
    else if (qtype == 32769) str = "DLV";
    else if (qtype == 39)    str = "DNAME";
    else if (qtype == 48)    str = "DNSKEY";
    else if (qtype == 43)    str = "DS";
    else if (qtype == 55)    str = "HIP";
    else if (qtype == 45)    str = "IPSECKEY";
    else if (qtype == 25)    str = "KEY";
    else if (qtype == 36)    str = "KX";
    else if (qtype == 29)    str = "LOC";
    else if (qtype == 15)    str = "MX";
    else if (qtype == 35)    str = "NAPTR";
    else if (qtype == 2)     str = "NS";
    else if (qtype == 47)    str = "NSEC";
    else if (qtype == 50)    str = "NSEC3";
    else if (qtype == 51)    str = "NSEC3PARAM";
    else if (qtype == 61)    str = "OPENPGPKEY";
    else if (qtype == 12)    str = "PTR";
    else if (qtype == 17)    str = "RP";
    else if (qtype == 24)    str = "SIG";
    else if (qtype == 6)     str = "SOA";
    else if (qtype == 33)    str = "SRV";
    else if (qtype == 44)    str = "SSHFP";
    else if (qtype == 32768) str = "TA";
    else if (qtype == 249)   str = "TKEY";
    else if (qtype == 52)    str = "TLSA";
    else if (qtype == 250)   str = "TSIG";
    else if (qtype == 16)    str = "TXT";
    else if (qtype == 256)   str = "URI";
    else if (qtype == 46)    str = "RRSIG";
    else if (qtype == 41)    str = "OPT";
    else if (qtype == 251)   str = "IXFR";
    else if (qtype == 252)   str = "AXFR";
    else if (qtype == 255)   str = "ALL";
    
    else {
        str = "";
        *err = UNKNOWN_TYPE;
    }

    return str;
}

/* 
* Converts string src to char* dest.
* Cest has to be allocated by caller function.
*/
void string_to_char(char* dest, string src) {
    const char* cstr = src.c_str();
    strcpy(dest, cstr);
}

// converts unsigned char* to string
string char_to_string(unsigned char* src) {
    stringstream ss;
    ss << src;
    string res = ss.str();
    return res;
}

// reads 16-bit number from data starting on read_ptr
string read_short(unsigned char** read_ptr)
{   
    short* num_ptr = (short*)*read_ptr;
    short num = ntohs(*num_ptr);
    stringstream ss;
    ss << num;
    string res = ss.str();

    *read_ptr += sizeof(short);   
    return res;
}

/* 
* Converts 32-bit network integer to host integer,
* then appends it to string dest with white space before.
*/
void int_net_to_string_host(string* dest, int net_num)
{   
    string tmp_str;
    int host_num = ntohl(net_num);
    stringstream ss;
    ss << host_num;
    tmp_str = ss.str();
    dest->append(" ");
    dest->append(tmp_str);
}

/*
* Reads constant fields SOA resource record starting on read_ptr.
* Returns these fields as string.
*/
string read_SOA(unsigned char** read_ptr)
{   
    string res;
    string tmp_str;
    SOA* rec = (SOA*)*read_ptr;

    int_net_to_string_host(&res, rec->serial);
    int_net_to_string_host(&res, rec->refresh);
    int_net_to_string_host(&res, rec->retry);
    int_net_to_string_host(&res, rec->expire);
    int_net_to_string_host(&res, rec->minimum);

    *read_ptr += sizeof(SOA);
    return res;
}

/* 
* Reads constant fields DNSKEY resource record starting on read_ptr.
* Returns these fields as string.
*/ 
string read_DNSKEY(unsigned char** read_ptr)
{   
    string res;

    DNSKEY *rec = (DNSKEY*)*read_ptr;
    rec->flags = ntohs(rec->flags);

    stringstream ss;
    ss << rec->flags << " " << (int)rec->protocol << " " << (int)rec->algorithm;
    res = ss.str();

    *read_ptr += sizeof(DNSKEY);
    return res;
}

/*
* Reads constant fields DS resource record starting on read_ptr.
* Returns these fields as string.
*/
string read_DS(unsigned char** read_ptr)
{   
    string res;

    DS *rec = (DS*)*read_ptr;
    rec->key_tag = ntohs(rec->key_tag);

    stringstream ss;
    ss << rec->key_tag << " " << (int)rec->algorithm << " " << (int)rec->digest_type;
    res = ss.str();
    *read_ptr += sizeof(DS);
    return res;
}

/*
* Computes seconds between '1 January 1970 00:00:00 UTC' and current time,
* then adds secs_elapsed to that time and converts it to time format string.
* For example: '1 January 1970 00:00:00 UTC' , secs_elapsed = 0, would return string "19700101000000".
*/   
string get_time(uint32_t secs_elapsed)
{      
    // referential time: 1 January 1970 00:00:00 UTC
    struct tm time_tm = {0}; // null structure before usage
    time_tm.tm_sec = secs_elapsed;
    time_tm.tm_min = 0;
    time_tm.tm_hour = 0;
    time_tm.tm_mday = 1;
    time_tm.tm_mon = 1;
    time_tm.tm_year = 70;

    mktime( &time_tm);      // normalize it
    int year = time_tm.tm_year + 1900;

    stringstream tmp_ss;
    tmp_ss << year << time_tm.tm_mon << time_tm.tm_mday << time_tm.tm_hour << time_tm.tm_min << time_tm.tm_sec;
    string expir = tmp_ss.str();

    return expir;
}

/* 
* Reads constant fields RRSIG resource record starting on read_ptr.
* Returns these fields as string.
*/
string read_RRSIG(unsigned char** read_ptr)
{
    string res;
    RRSIG *rec = (RRSIG*)*read_ptr;
    stringstream ss;
    int err = 0;
    int type_num = ntohs(rec->type_covered);
    string type = get_qtype_string(type_num, &err);

    uint32_t expir_secs = ntohl(rec->sign_expir);
    uint32_t incept_secs = ntohl(rec->sign_incept);

    string expir = get_time(expir_secs);
    string incept = get_time(incept_secs);

    ss << type << " " << (int)rec->algorithm << " "
       << (int)rec->labels << " " << ntohl(rec->original_ttl) << " "
       << expir << " " << incept << " " 
       << ntohs(rec->key_tag);
       res = ss.str();

    *read_ptr += sizeof(RRSIG);

    return res;
}

// reads base64 decoded bytes and converts it to string
string read_base64(unsigned char* read_ptr, unsigned char* temp, int data_len)
{
    for (int j = 0; j < data_len; j++)
    {
       temp[j] = read_ptr[j];
    }
    temp[data_len] = '\0';

    string encoded = base64_encode(temp, data_len);
    return encoded;
}

/* 
* Converts string to hexadecimal format.
* source: https://stackoverflow.com/questions/3381614/c-convert-string-to-hexadecimal-and-vice-versa
*/
std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

/*
* Checks if bit 'k' of integer 'n' is set.
* source: https://www.geeksforgeeks.org/check-whether-k-th-bit-set-not/
*/
bool isKthBitSet(int n, int k) 
{ 
    if (n & (1 << (k - 1))) 
        return true; 
    else
        return false; 
}

/* 
* Reads first DNS resource record answer.
* Appends printable result string to res_data.
*/
int read_answers(Res_record* answer, string* res_data, size_t querie_size, u_char* buff, int ancount)
{   
    // pointer to data (DNS resource record)
    unsigned char *read_ptr = &buff[querie_size];    

    for (int cnt = 0; cnt < ancount; cnt++)
    {   
        string answer_str = "";
        int stop = 0;
        // read Res_record.name
        read_domain_name(answer->name, read_ptr, buff, &stop);
        read_ptr += stop; // offset read_ptr after 'name' -> to 'resource'
        
        // read Res_record.resource
        answer->resource = (R_metadata*)read_ptr; // resource starts on read_ptr
        int answer_type = ntohs(answer->resource->type);
        read_ptr += sizeof(R_metadata); // offset read_ptr after R_metadata -> to 'rdata'

        int data_len = ntohs(answer->resource->rdlength);
        // If the type is not supported, skip the answer
        if (answer_type != Type_NS     && answer_type != Type_CNAME && answer_type != Type_A     &&
            answer_type != Type_AAAA   && answer_type != Type_TXT   && answer_type != Type_SOA   && 
            answer_type != Type_DNSKEY && answer_type != Type_MX    && answer_type != Type_RRSIG &&
            answer_type != Type_DS     && answer_type != Type_NSEC)
        {   
            read_ptr += data_len;
            continue;
        }
        

        int alloc_size = 0;
        bool alloc_domain = false;
        // NS and CNAME types contain only a domain name
        if (answer_type == Type_NS || answer_type == Type_CNAME) {
            alloc_size = name_size;
        /* 
        * A, AAAA and TXT have only one record (IP address or character string), 
        * These dont need any space for white characters between records, so we can allocate
        * just their actual length.
        */
        } else if (answer_type == Type_A || answer_type == Type_AAAA || answer_type == Type_TXT) {
            alloc_size = ntohs(answer->resource->rdlength) + 1; // +1 for ending zero byte
        }   

        char* domain;
        // These types have a domain name in their data
        if (answer_type == Type_SOA || answer_type == Type_MX || answer_type == Type_NSEC ||
            answer_type == Type_RRSIG) 
        {
            alloc_domain = true;
            domain = (char*) malloc(name_size+1);
            if (domain == NULL) {
                fprintf(stderr, "Malloc failed\n");
                return ERR;
            }
        }

        string tmp_str;
        unsigned char* temp;
        bool alloc_temp = false;

        // These types need temporary memory to process data
        if (answer_type == Type_NSEC || answer_type == Type_DNSKEY || answer_type == Type_RRSIG ||
            answer_type == Type_DS) 
        {
            alloc_temp = true;
            temp = (unsigned char*) malloc(data_len + 1);
            if (temp == NULL) {
                fprintf(stderr, "Malloc failed\n");
                free(temp);
                return ERR;
            }
        }

        // allocate enough space for data types, where reading bytes one by one is needed
        if (alloc_size != 0) {
            answer->rdata = (char*) malloc(alloc_size);
            if (answer->rdata == NULL) {
                fprintf(stderr, "Malloc failed\n");
                return ERR;
            } 
        }    
        
        /* 
        * If type is one of these, there will be no need to further cut data.
        * It can be in fixed sized array.
        * We just read them and convert to readable format.
        */
        if (answer_type == Type_A || answer_type == Type_AAAA || answer_type == Type_TXT) {
            if (answer_type == Type_TXT) {
                // TXT field has a special "TXT length" one byte field
                uint8_t* txt_len_ptr = (uint8_t*)read_ptr;
                uint8_t txt_len = *txt_len_ptr;
                read_ptr += 1;
                data_len = txt_len;
            }
            // read the whole RDATA field
            for (int j = 0; j < data_len; j++)
            {
               answer->rdata[j] = read_ptr[j];
            }
            answer->rdata[data_len] = '\0';
            read_ptr += data_len;
            
            // read Type A, AAAA converts IP address to readable format
            if (answer_type == Type_A) {
                struct sockaddr_in addr4;
                addr4.sin_addr.s_addr = *(long*)answer->rdata;        
                answer_str.append(inet_ntoa(addr4.sin_addr));

            } else if (answer_type == Type_AAAA) {
                char str[INET6_ADDRSTRLEN];
                if (inet_ntop(AF_INET6, answer->rdata, str, INET6_ADDRSTRLEN) == NULL) {
                    perror("inet_ntop");
                    free(answer->rdata);
                    return ERR;
                }
                answer_str.append(str);

            } else if (answer_type == Type_TXT) {
                // TXT data - just a character string - already read before
                answer_str.append("\"");
                answer_str.append(answer->rdata);
                answer_str.append("\"");
            }

        } else if (answer_type == Type_SOA) {
            // read Mname and Rname
            for (int i = 0; i < 2; i++)
            {
                answer_str.append(read_domain_name(domain, read_ptr, buff, &stop));
                read_ptr += stop;
                answer_str.append(" ");            
            }
            // read constant sized fields of SOA
            tmp_str = read_SOA(&read_ptr);        
            answer_str += tmp_str;

        } else if (answer_type == Type_NS || answer_type == Type_CNAME) {
            answer_str.append(read_domain_name(answer->rdata, read_ptr, buff, &stop));
            read_ptr += stop;
            answer_str.append(" ");

        } else if (answer_type == Type_MX) {
            // read preference field
            answer_str += read_short(&read_ptr);                 

            // read Exchange
            tmp_str.append(read_domain_name(domain, read_ptr, buff, &stop));
            read_ptr += stop;
            answer_str.append(" ");
            answer_str += tmp_str;

        } else if (answer_type == Type_DNSKEY) {
            string public_key;
            // read constant sized fields of DNSKEY
            tmp_str = read_DNSKEY(&read_ptr);

            answer_str += tmp_str;
            answer_str.append(" ");

            // read Public Key
            int rest_size = data_len - (sizeof(DNSKEY));
            answer_str += read_base64(read_ptr, temp, rest_size);
            read_ptr += rest_size;

        } else if (answer_type == Type_NSEC) {
            answer_str.append(read_domain_name(domain, read_ptr, buff, &stop));
            read_ptr += stop;

            // read undefined number of Type Bit Maps
            int to_read = data_len - stop;
            while (to_read > 0) {
                //Type Bit Maps Field Window Block
                uint8_t block_num = read_ptr[0];
                read_ptr++;
                to_read--;

                // length of Type Bit Maps Field
                uint8_t len = read_ptr[0]; 
                read_ptr++;
                to_read--;

                int ref[len];
                // reads bits of Type Bit Maps Field
                for (int i = 0; i < len; ++i)
                {
                    ref[i] = read_ptr[0];
                    read_ptr++;
                    to_read--;
                }

                // get rr types from Type Bit Maps Field        
                string type;
                for (int i = 0; i < len; ++i)
                {   
                    // read from end of byte - to convert network to host endian
                    for (int j = 7; j >= 0; j--)
                    {
                        if (isKthBitSet(ref[i], j)) {
                             // convert number from network to host endian and add byte number
                            int qtype = (8-j) + 8*i;
                            qtype += 256 * block_num;
                            int err = UNKNOWN_TYPE;
                            type = get_qtype_string(qtype, &err);
                            if (err == 0) {
                                answer_str.append(" ");
                                answer_str += type;
                            }                                                 
                        }
                    }       
                }
            }           

        } else if (answer_type == Type_DS) {
            // read constant sized fields of DS
            answer_str += read_DS(&read_ptr);

            int rest_len = data_len - sizeof(DS);
            // read Digest (hexadecimal format)
            for (int j = 0; j < rest_len; j++)
            {
                temp[j] = read_ptr[j];
            }
            temp[rest_len] = '\0';
            read_ptr += rest_len;

            answer_str.append(" ");
            string data = char_to_string(temp);
            answer_str.append(string_to_hex(data));

        } else if (answer_type == Type_RRSIG) {
            // read constant size fields if RRSIG
            answer_str += read_RRSIG(&read_ptr);

            answer_str.append(" ");
            // read Signer's Name
            answer_str.append(read_domain_name(domain, read_ptr, buff, &stop));
            read_ptr += stop;

            answer_str.append(" ");

            // read Signature Field
            int rest_size = data_len - (sizeof(RRSIG) + stop);            
            answer_str += read_base64(read_ptr, temp, rest_size);
            read_ptr += rest_size;
        }

        // clean memory
        if (alloc_size != 0) free(answer->rdata);
        if (alloc_domain == true) free(domain);
        if (alloc_temp == true) free(temp);

        string type_str;
        int err = 0;
        type_str = get_qtype_string(answer_type, &err);
        if (err) {
            return ERR;
        } 
        // fill the answer
        res_data->append(answer->name);
        res_data->append(" ");
        res_data->append(type_str);
        res_data->append(" ");
        res_data->append(answer_str);
        res_data->append("\n");
    } // end of for
    return 0;
}


/*
* Checks for error codes in response.
* If there is an answer, reads it and returns in string.
*/
string parse_response(size_t querie_size, Header* header, u_char* buff)
{   
    string res_str = "";
    if (header->rcode != 0) {
        const char* err_msg;
        if      (header->rcode == 1) err_msg = "Format error\n";
        else if (header->rcode == 2) err_msg = "Server failure\n";
        else if (header->rcode == 3) err_msg = "Name error\n";
        else if (header->rcode == 4) err_msg = "Not Implemented\n";
        else if (header->rcode == 5) err_msg = "Refused\n";
        else                         err_msg = "Unknown error code\n"; 
        fprintf(stderr, "Response code error: %s", err_msg);
        return "";
    }
    int ancount = ntohs(header->ancount); // number of answer entries

    if (ancount > 0) {
        Res_record answer;
        int err = 0;
        err = read_answers(&answer, &res_str, querie_size, buff, ancount);
        if (err != 0 || res_str.length() == 0) return "";
        res_str.erase(res_str.length()-1); // delete the last \n character
    } else {
        return "";
    } 
    
    return res_str;
}

/* 
* Parses packet to find DNS resource record data.
* Returns first answer in format string.
*/
string parse_dns_packet(u_char* buff)
{   
    string res_str;
    char* qname;

    // skip queries field
    qname =(char*)&buff[sizeof(Header)];
    size_t qname_size = (strlen((const char*)qname) + 1);
    size_t header_qname_size = sizeof(Header) + qname_size;
    // get queries size, so we can skip it later to read the first answer
    size_t querie_size = header_qname_size + sizeof(Querie_info);

    // read Header 
    // Same for TCP and UDP, lenght in TCP was already skipped.
    Header *header;
    header = (Header*) buff;

    // check header and read all the answers
    res_str = parse_response(querie_size, header, buff);

    return res_str;
}