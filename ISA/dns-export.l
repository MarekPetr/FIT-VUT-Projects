.TH DNS_EXPORT
.SH NAME
dns_export \- export DNS protocol data
.SH SYNOPSIS
.B dns_export
[\fIOPTION\fR]
.SH DESCRIPTION
Sniff on network interface or file for DNS response packets. Then parse DNS protocol data and export statistics using Syslog central logging server.
.SH OPTIONS
.TP
.BR \-r " "\fIfile.pcap\fR
parse file.pcap
.TP
.BR \-i " "\fIinterface\fR
listen on network interface and parse DNS traffic
.TP
.BR \-s " "\fIsyslog-server\fR
hostname/ipv4/ipv6 syslog server address
.TP
.BR \-t " "\fIseconds\fR
time period between sending of statistics to syslog server