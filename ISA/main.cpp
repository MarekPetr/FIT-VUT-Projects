#include "headers.hpp"

pcap_t *handle;         // packet capture handle
string records_str;     // formated dns asnwers       
Arguments args;         // arguments

int parse_args(int argc ,char *argv[]);

/*
* This program sniffs packets on interface or from file - depends on command line arguments.
* Then sends them to syslog server and/or prints computed statistics to stdout.
*/
int main(int argc, char **argv)
{  
    args.timeout = 60; // default value

    // parse arguments
    int err = parse_args(argc, argv);
    if (err != 0) return err;

    // if server is set, get a socket
    if (args.server_set) {
        err = get_socket();
        if (err != 0) return err;
    }

    // sniff on interface or pcap file
    err = sniff();
    if (err) {
        close(args.comm_socket);     // close socket
        freeaddrinfo(args.servinfo); // free address info allocated by get_socket()
        return err;
    }
    
    /*
    * If we sniffed on pcap file, we need to print
    * and/or send statistics to syslog server.
    */
    if (args.run_flag == FILE) {
        string result = "";
        // if there are some records, compute statistics
        if (records_str.length() > 0) {        
            result = get_stats();
            // is server is not set, just print result
            if (args.server_set == false) {
                cout << result << endl; 
            // server is set, send statistics          
            } else {
                err = send_syslog_line(result);
                close(args.comm_socket);
                freeaddrinfo(args.servinfo);

                if (err != 0) {
                    return err;
                }
            }
        }
    }

    return 0;  
}

// Ensures that each parameter is used only once
int used_twice(int param) {
    if (param) {
        fprintf(stderr, "Each parameter can be used only once.\n");
        return ERR_ARG;
    }
    return 0;
}

// prints usage message
void print_usage() {
    fprintf(stderr, "Usage: dns-export [-r file.pcap] [-i interface] [-s syslog-server] [-t seconds]\n");
}

// Parses command line arguments and save them to global Arguments structure
int parse_args(int argc ,char *argv[])
{   
    int opt;
    int r = 0;
    int i = 0;
    int s = 0;
    int t = 0;
    args.server_set = false;

    while ((opt = getopt(argc, argv, "r:i:s:t:")) != -1) {
        switch(opt) {
            case 'r':
                if (used_twice(r)) return ERR_ARG;
                r = 1;
                strcpy(args.pcap_file, optarg);
                break;
            case 'i':
                if (used_twice(i)) return ERR_ARG;
                i = 1;
                strcpy(args.device, optarg);
                break;
            case 's':
                if (used_twice(s)) return ERR_ARG;
                s = 1;
                strcpy(args.dns_server, optarg);
                args.server_set = true;
                break;
            case 't':
                if (used_twice(t)) return ERR_ARG;
                t = 1;
                for (unsigned int i = 0; i < strlen(optarg); i++)
                {
                   if (!isdigit(optarg[i])) {
                        fprintf(stderr, "Timeout has to be a number\n");
                        return ERR_ARG;
                    } 
                }
                args.timeout = atoi(optarg);
                break;
            default:
                print_usage();
                return ERR_ARG;
        }
    }
    // check if there is an unknown argument
    if (argc > optind) {
        print_usage();
        return ERR_ARG;
    }

    if (r && i) {
        fprintf(stderr, "Bad arguments. -r can not be used with -i option.\n");
        print_usage();
        return ERR_ARG;

    }

    if (r && t) {
        fprintf(stderr, "Bad arguments. -r can not be used with -t option.\n");
        print_usage();
        return ERR_ARG;
    }

    else if (r) args.run_flag = FILE;
    else if (i) args.run_flag = DEVICE;
    else {
        fprintf(stderr, "Either -r or -i option has to be used.\n");
        print_usage();
        return ERR_ARG;
    }

    return 0;
}

/*
* Parses records_str global string and computes statistics.
* Appends these statistics to string returned by these function.
*/
string get_stats() {
    string record;
    istringstream orig(records_str);
    string orig_line;
    bool found = false;

    /*
    * 1. Go through every orig line and if it is not found in record string, append it.
    * 2. Go through every rec line and compare it with every orig line,
    *    if it is the same, increment cnt, after whole orig string was processed,
    *    append cnt to the rec line.
    */

    //  Go through every orig line and if it is not found in record string, append it.
    while (getline(orig, orig_line)) {
        istringstream rec(record);
        string rec_line;

        while(getline(rec, rec_line)) {
            if (orig_line.compare(rec_line) == 0) {
                found = true;
            }   
        }
        if (!found) {
            record.append(orig_line);
            record.append("\n");
        }
        found = false;
    }  

    // 
    istringstream rec2(record);
    string rec2_line;    
    string orig2_line;
    string with_stats;

    int cnt = 0;
    // go through every rec line and compare it with every orig line
    while (getline(rec2, rec2_line)) {
        cnt = 0;
        istringstream orig2(records_str);   
        while (getline(orig2, orig2_line)) {
            // if it is the same, increment cnt
            if (rec2_line.compare(orig2_line) == 0) {
                cnt++;
            }
        }
        //after whole orig string was processed, append cnt to the rec line
        with_stats.append(rec2_line);
        with_stats.append(" ");
        stringstream ss;
        ss << (cnt--);
        with_stats.append(ss.str());
        with_stats.append("\n");
    }

    /*if (with_stats.length() > 0) {
         // remove the last 'end of line' (EOL) character
        with_stats.erase(with_stats.length()-1);
    }*/
    return with_stats;
}

// calculates statistics and print them to stdout
void print_statistics() {
    string result;
    result = "";
    
    if (records_str.length() > 0) {        
        result = get_stats();
    }
    cout << result << endl;
}