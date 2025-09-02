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

    struct sockaddr_in dest_addr, src_addr;

    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(ip);

    bzero(&src_addr, sizeof(src_addr));
    src_addr.sin_family = AF_INET;
    src_addr.sin_addr.s_addr = INADDR_ANY;
    src_addr.sin_port = htons((getpid() & 0xFFFF) | (1 << 15));

    if (bind(send_sock, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind");
        close(send_sock);
        free(ip);
        free(args.address);
        close(recv_sock);
        return EXIT_FAILURE;
    }

    printf("traceroute to %s (%s), %d hops max\n", args.address, ip, args.max_hops);

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
        int ip_printed = 0;
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
            fd_set readfds;
            FD_ZERO(&readfds);

            FD_SET(recv_sock, &readfds);
            struct timeval tv;
            tv.tv_sec = args.timeout;
            tv.tv_usec = 0;
            
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
                    struct ip *ip_hdr = (struct ip *)recv_packet;
                    struct icmp *icmp_hdr = (struct icmp *)(recv_packet + (ip_hdr->ip_hl * 4));
                    if (icmp_hdr->icmp_type == ICMP_UNREACH || icmp_hdr->icmp_type == ICMP_TIMXCEED) {
                        struct ip *orig_ip_hdr = (struct ip *)(recv_packet + (ip_hdr->ip_hl * 4) + 8);
                        int orig_ip_hdr_len = orig_ip_hdr->ip_hl * 4;

                        struct udphdr *orig_udp_hdr = (struct udphdr *)((char *)orig_ip_hdr + orig_ip_hdr_len);

                        in_port_t src_port = (orig_udp_hdr->uh_sport);
                        in_port_t dst_port = (orig_udp_hdr->uh_dport);
                        if (src_port != src_addr.sin_port || dst_port != dest_addr.sin_port)
                            continue;
                        gettimeofday(&recv, NULL);
                        char *recv_ip = inet_ntoa(ip_hdr->ip_src);
                        if (ip_printed == 0) {
                            printf("%s  ", recv_ip);
                            ip_printed = 1;
                        }
                        float ms = get_ms(send, recv);
                        printf("%.3f ms  ", ms);    
                        if (icmp_hdr->icmp_type == ICMP_UNREACH && icmp_hdr->icmp_code == ICMP_UNREACH_PORT && i == 2) loop = 0;
                    }
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