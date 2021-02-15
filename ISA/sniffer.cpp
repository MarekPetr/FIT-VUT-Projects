#include "headers.hpp"

#define SIZE_ETHERNET 14
#define SIZE_UDP 8

// captures packets on interface or from file
int sniff()
{
    int where = args.run_flag;
    char errbuf[PCAP_ERRBUF_SIZE];                  // error buffer

    char filter_exp[] = "src port 53";              // filter expression
    struct bpf_program fp;                          // compiled filter program (expression)
    bpf_u_int32 mask;                               // subnet mask
    bpf_u_int32 net;                                // ip
    
    if (where == DEVICE) {
        // get network number and mask associated with capture device
        if (pcap_lookupnet(args.device, &net, &mask, errbuf) == -1) {
            fprintf(stderr, "Couldn't get netmask for device %s: %s\n",
                args.device, errbuf);
            net = 0;
            mask = 0;
            return ERR;
        }

        // print capture info
        printf("INFO: PID : %d\n", getpid());
        
        handle = NULL;
        
       // open capture device
        handle = pcap_open_live(args.device, SNAP_LEN, 1, 1000, errbuf);
        if (handle == NULL) {
            fprintf(stderr, "Couldn't open device %s: %s\n", args.device, errbuf);
            return ERR;
        }
        set_sig_handlers(args.timeout);

    } else {
        // open capture file for offline processing
        handle = pcap_open_offline(args.pcap_file, errbuf);
        if (handle == NULL) {
            cout << "pcap_open_offline() failed: " << errbuf << endl;
            return ERR;
        }  
    }    

    // make sure we're capturing on an Ethernet device
    if (pcap_datalink(handle) != DLT_EN10MB) {
        fprintf(stderr, "%s is not an Ethernet\n", args.device);
        return ERR;
    }

    // compile the filter expression 
    if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n",
            filter_exp, pcap_geterr(handle));
        return ERR;
    }

    // apply the compiled filter
    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter %s: %s\n",
            filter_exp, pcap_geterr(handle));
        return ERR;
    }

    const u_char *packet;
    struct pcap_pkthdr header;
    struct ether_header *eptr;
    struct ip *ip_hdr;
    const struct udphdr* udp_hdr;   // pointer to the beginning of UDP header
    const struct tcphdr *tcp_hdr;   // pointer to the beginning of TCP header
    u_int size_ip;
    u_int size_tcp;

    u_char* payload;
    u_int size_payload;
    int dataLength = 0;
    string dataStr;

    int pck_cnt = 0; // packet counter

    string payload_str; // contains payloads from segmented answers
    u_int curr_seq = 0;
    u_int next_seq = 0;
    int flag;
    bool is_first = false;

    while ((packet = pcap_next(handle,&header)) != NULL) {

        eptr = (struct ether_header *) packet;
        if (ntohs(eptr->ether_type) == ETHERTYPE_IP) {
            ip_hdr = (struct ip*) (packet+SIZE_ETHERNET);        // skip Ethernet header
            size_ip = ip_hdr->ip_hl*4;                           // length of IP header

            if (ip_hdr->ip_p == IPPROTO_UDP) {
                pck_cnt++;
                udp_hdr = (struct udphdr *) (packet+SIZE_ETHERNET+size_ip); // skip IP header

                 // skip UDP header -> pointer to the payload
                payload = (u_char*)(packet + SIZE_ETHERNET + size_ip + SIZE_UDP);         
                dataStr = parse_dns_packet(payload);
                if (dataStr.length() > 1) {             
                    records_str.append(dataStr);
                    records_str.append("\n");
                }
            } else if (ip_hdr->ip_p == IPPROTO_TCP) {
                pck_cnt++;
                tcp_hdr = (struct tcphdr *) (packet+SIZE_ETHERNET+size_ip); // pointer to the TCP header
                size_tcp = tcp_hdr->th_off*4;
                
                if (size_tcp < 20) {
                    fprintf(stderr,"Invalid TCP header length: %u bytes\n", size_tcp);
                    continue;
                }                
                // define/compute tcp payload (segment) offset
                payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);                
                // compute tcp payload (segment) size
                size_payload = ntohs(ip_hdr->ip_len) - (size_ip + size_tcp);
                
                // if there is no payload, skip packet
                if (size_payload <= 0) {
                    continue;
                }

                curr_seq = ntohl(tcp_hdr->th_seq);
                size_t size_short = sizeof(u_short);
                /*
                * check if packet is part of currently processes stream
                * if not, flush the buffer (payload_str) and start reading new one
                */
                if (next_seq != 0) {
                    if (next_seq != curr_seq) {
                        payload_str = "";
                        is_first = true;
                    } else {
                        is_first = false;
                    }
                }

                if (is_first) {
                      /* 
                    * Unlike UDP heade TCP two byte Length field, we have to read it here
                    * so only one structure can be used to read DNS messages.
                    */
                    u_short* len_ptr = (u_short*)payload;
                    u_short len = ntohs(*len_ptr);
                    payload += size_short;
                    size_payload -= size_short;
                }

                // append current payload to payload string
                payload_str.append((char*)payload, size_payload);
                // PUSH flag signalizes end of one TCP stream
                int flag = tcp_hdr->th_flags & TH_PUSH;
                if (flag == TH_PUSH) {                        
                    u_char* cstr = (u_char*) payload_str.c_str();

                    dataStr = parse_dns_packet(cstr);
                    if (dataStr.length() > 1) {             
                        records_str.append(dataStr);
                        records_str.append("\n");
                    }
                    payload_str = "";
                    pck_cnt++;
                }
                /*
                * next sequence number must equal to current sequence number plus payload size
                * if not, packet does not belong to the same stream
                * + sizeof(u_short) to compensate length field (in TCP DNS header)
                */
                next_seq = curr_seq + size_payload + size_short; 
            } // end of TCP if
        } // end of ETHERTYPE if        
    } // end of while

    // cleanup
    pcap_freecode(&fp);
    pcap_close(handle);

return 0;
}
