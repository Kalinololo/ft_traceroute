#include "ft_traceroute.h"

int loop = 1;

void handle_quit() 
{
    loop = 0;
}

int main(int ac, char **av)
{
    if (ac < 2)
    {
        printf("%s: missing host operand\nTry './ft_traceroute --help' for more information.\n", av[0]);
        return 1;
    }
    ParsedArgs args = parse_args(ac, av);
    if (args.usage) {
        if (args.address) free(args.address);
        return print_usage(args, av[0]);
    }
    if (getuid() != 0)
    {
        free(args.address);
        printf("%s: You have to be superuser to use this program\n", av[0]);
        return 1;
    }
    char *ip = resolve_hostname(args.address);
    if (!ip) {
        free(args.address);
        printf("%s: unknown host\n", av[0]);
        return 1;
    }
    int recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (recv_sock < 0) 
    {
        free(ip);
        free(args.address);
        perror("socket");
        return 1;
    }

    int send_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (send_sock < 0) 
    {
        free(ip);
        free(args.address);
        close(recv_sock);
        perror("socket");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = handle_quit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // SA_RESTART to 0 so blocking operation like recvfrom can be interrupted
    sigaction(SIGINT, &sa, NULL);

    struct sockaddr_in dest_addr;

    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(ip);

    printf("traceroute to %s (%s), %d hops max\n", args.address, ip, args.max_hops);

    struct timeval tv;
    tv.tv_sec = args.timeout;
    tv.tv_usec = 0;

    fd_set readfds;
    FD_ZERO(&readfds);

    int error = 1;

    int ttl = 1;

    while (ttl <= args.max_hops && loop)
    {
        if (setsockopt(send_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1) {
            printf("ft_traceroute: setsockopt: %s\n", strerror(errno));
            error = 0;
            break;
        }
        printf(" %2d   ", ttl);
        dest_addr.sin_port = htons(args.port + ttl - 1);
        int i = 0;
        while (i < args.packet_per_hop && loop)
        {
            char recv_packet[512];
            bzero(recv_packet, 512);
            struct timeval send, recv;

            gettimeofday(&send, NULL);
            if (sendto(send_sock, PAYLOAD, sizeof(PAYLOAD), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) 
            {
                printf("ft_traceroute: sendto: %s\n", strerror(errno));
                error = 0;
                break;
            }
            FD_SET(recv_sock, &readfds);

            int ret = select(recv_sock + 1, &readfds, NULL, NULL, &tv);
            if (ret == -1) {
                if (errno == EINTR) break; 
                perror("select");
                error = 0;
                break;
            } else if (ret == 0) {
                if (i > 0) printf(" ");
                printf("* ");
                fflush(stdout);
            } else {
                ssize_t n = recvfrom(recv_sock, &recv_packet, sizeof(recv_packet), 0, NULL, NULL);
                if (n > 0) {
                    gettimeofday(&recv, NULL);
                    struct ip *ip_hdr = (struct ip *)recv_packet;
                    struct icmp *icmp_hdr = (struct icmp *)(recv_packet + (ip_hdr->ip_hl << 2));

                    char *recv_ip = inet_ntoa(ip_hdr->ip_src);
                    if (i == 0) printf("%s  ", recv_ip);
                    float ms = get_ms(send, recv);
                    printf("%.3f ms  ", ms);

                    if (icmp_hdr->icmp_type == ICMP_UNREACH && icmp_hdr->icmp_code == ICMP_UNREACH_PORT && i == 2) loop = 0;
                }
            }
            i++;
        }
        printf("\n");
        if (!loop) break;
        if (!error) break;
        ttl++;
    }
    close(recv_sock);
    close(send_sock);
    free(args.address);
    free(ip);
    return 0;
}