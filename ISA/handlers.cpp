#include "headers.hpp"

// handles alarm - sends statistics after timeout has elapsed
void alarm_handler(int sig)
{    
    // make sure server is ready to receive
    if (args.server_set) {
        string result = "";
        if (records_str.length() > 0) {                  
            result = get_stats();        
            send_syslog_line(result);
        }
        // reset new alarm after this one
        alarm(args.timeout);
    } 

}

/* 
* Handles signals.
* On SIGUSR1 prints statistics.
*/
void sig_handler(int sig)
{
    if (sig == SIGUSR1) {
        print_statistics();     
    } else {
        pcap_breakloop(handle);
    }
}

/*
* Sets signal handlers, alarm timeout.
* Triggers alarm_handler on timeout.
*/
void set_sig_handlers(int timeout)
{   
    if (args.server_set) {
        alarm(timeout);
        signal(SIGALRM, alarm_handler);
    }
    
    struct sigaction act;
    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act, 0);
    sigaction(SIGUSR1, &act, 0);
    sigaction(SIGTERM, &act, 0);
}