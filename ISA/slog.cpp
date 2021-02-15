#include "headers.hpp"

#define PORT "514"

/*
* Sends message msg to connected syslog server.
* Socket has to be provided in args.comm_socket.
*/
int sendto_syslog_server(string msg)
{   
    int err = 0;

    // current time structure
    char cur_time[86] = "";
    time_t mytime = {0};
    struct tm* ptm;
    ptm = gmtime(&mytime);

    // compute miliseconds
    timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 1000;

    // store time in appropriate time format
    char buffer [80] = "";
    strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S", gmtime(&curTime.tv_sec));

    // add miliseconds to time
    char currentTime[84] = "";
    sprintf(currentTime, "%s.%dZ", buffer, milli);

    // get ip or hostname
    string hostname = get_host();

    // create message
    string res;
    res.append("<134>1 ");
    res.append(currentTime);
    res.append(" ");
    res.append(hostname + " dns-export --- " + msg);
    
    //send message
    const char* c_msg = res.c_str();
    int numbytes = 0;
    if ((numbytes = sendto(args.comm_socket, c_msg, strlen(c_msg), 0,
             args.p->ai_addr, args.p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        close(args.comm_socket);
        freeaddrinfo(args.servinfo);
        return 1;
    }
    return 0;
}

// cut string msg to lines and send them one by one to syslog server
int send_syslog_line(string msg)
{   
    int err = 0;
    istringstream orig(msg);
    string line;
    while (getline(orig, line)) {
        err = sendto_syslog_server(line);
        if (err) {
            fprintf(stderr, "failed to send statistics\n");
            return err;
        }
    }
    return 0;
}

/*
* Gets socket to server and saves it 
* in Arguments structure for future use.
*/
int get_socket()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
    hints.ai_protocol = 0;          // Any protocol
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    const char* address = (const char*) args.dns_server;

    if ((rv = getaddrinfo(address, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return ERR;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return ERR;
    }
    args.servinfo = servinfo;
    args.p = p;
    args.comm_socket = sockfd;
    return 0;

}

// reads IP adress from sockaddr structure
// source https://stackoverflow.com/questions/1824279/how-to-get-ip-address-from-sockaddr/9212542
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// gets IP address or hostname of this host
string get_host() {
    string res;

    struct addrinfo hints, *info, *p;
    int gai_result;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    int err = false;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //either IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if ((gai_result = getaddrinfo(hostname, "http", &hints, &info)) != 0) {
        err = true;
    }

    char ip[INET6_ADDRSTRLEN]; // IP adress
    for(p = info; p != NULL; p = p->ai_next) {
        // save IP address from sockaddr structure to 's' array
        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), ip, sizeof ip);
    }

    // use IP address if found, else use domain name instead
    if (err) res.append(hostname);
    else res.append(ip);

    freeaddrinfo(info);
    return res;
}