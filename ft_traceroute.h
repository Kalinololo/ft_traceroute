#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>
#include <signal.h>

#define PAYLOAD "0123456789abcdef0123456789abcde"

typedef struct ParsedArgs {
    char *address;
    int usage;
    char error[100];
    int max_hops;
    int packet_per_hop;
    int port;
    int timeout;
} ParsedArgs;

// -- Parse --
ParsedArgs parse_args(int ac, char **av);
int print_usage(ParsedArgs args, char *prog);

// -- Traceroute --
char *resolve_hostname(char *hostname);
float get_ms(struct timeval start, struct timeval end);
char *get_host(char *ip);

#endif