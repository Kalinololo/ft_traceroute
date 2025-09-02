#include "ft_traceroute.h"

char *resolve_hostname(char *hostname) 
{
    struct addrinfo hints, *result, *elem;
    char *ip = malloc(INET_ADDRSTRLEN);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) 
    {
        free(ip);
        return NULL;
    }
    elem = result;
    while (elem != NULL)
    {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)elem->ai_addr;
        void *addr = &(ipv4->sin_addr);
        if (inet_ntop(AF_INET, addr, ip, INET_ADDRSTRLEN) != NULL) {
            freeaddrinfo(result);
            return ip;
        }
        elem = elem->ai_next;
    }
    freeaddrinfo(result);
    free(ip);
    return NULL;
}

float get_ms(struct timeval start, struct timeval end)
{
    return ((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0);
}

char *get_host(char *ip)
{
    struct hostent *host;
    struct in_addr addr;
    addr.s_addr = inet_addr(ip);
    host = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);
    if (host) return host->h_name;
    return NULL;
}