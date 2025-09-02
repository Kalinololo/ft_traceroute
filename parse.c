#include "ft_traceroute.h"

// -v verbose
// -? usage

int is_valid_number(char *str, int neg, int flo)
{
    int i = 0;
    while (str[i])
    {
        if (str[i] < '0' || str[i] > '9' || (i == 0 && str[i] == '-' && !neg) || (str[i] == '.' && !flo))
            return 0;
        i++;
    }
    return 1;
}

ParsedArgs missing_arg(ParsedArgs args, char param)
{
    args.usage = 1;
    sprintf(args.error, "option requires an argument -- '%c'", param);
    return args;
}

ParsedArgs invalid_arg(ParsedArgs args, char *param)
{
    args.usage = 1;
    sprintf(args.error, "invalid value (`%s' near `%s')", param, param);
    return args;
}

ParsedArgs parse_args(int ac, char **av)
{
    ParsedArgs args;
    args.address = NULL;
    args.usage = 0;
    args.max_hops = 64;
    args.packet_per_hop = 3;
    args.port = 33434;
    args.timeout = 3;
    bzero(args.error, 100);
    int i = 1;
    while (i < ac)
    {
        if (!strcmp(av[i], "--help"))
            args.usage = 1;
        else if (!strcmp(av[i], "-m")) // Max hops
        {
            char *value = av[++i];
            if (!value || !strlen(value)) return missing_arg(args, 'm');
            if (!is_valid_number(value, 0, 0)) return (sprintf(args.error, "invalid hops value '%s'", value), args.usage = 1, args);
            long lValue = atol(value);
            if (lValue <= 0 || lValue > 255) return (sprintf(args.error, "invalid hops value '%s'", value), args.usage = 1, args);
            args.max_hops = lValue;
        }
        else if (!strcmp(av[i], "-q")) // Probes number
        {
            char *value = av[++i];
            if (!value || !strlen(value)) return missing_arg(args, 'q');
            if (!is_valid_number(value, 0, 0)) return invalid_arg(args, value);
            long lValue = atol(value);
            if (lValue <= 0 || lValue > 10) return (sprintf(args.error, "number of tries should be between 1 and 10"), args.usage = 1, args);
            args.packet_per_hop = lValue;
        }
        else if (!strcmp(av[i], "-p")) // Starting port
        {
            char *value = av[++i];
            if (!value || !strlen(value)) return missing_arg(args, 'p');
            if (!is_valid_number(value, 0, 0)) return (sprintf(args.error, "invalid port number '%s'", value), args.usage = 1, args);
            long lValue = atol(value);
            if (lValue <= 0 || lValue > 65536) return (sprintf(args.error, "invalid port number '%s'", value), args.usage = 1, args);
            args.port = lValue;
        }
        else if (!strcmp(av[i], "-w")) // Wait time
        {
            char *value = av[++i];
            if (!value || !strlen(value)) return missing_arg(args, 'w');
            if (!is_valid_number(value, 0, 0)) return (sprintf(args.error, "ridiculous waiting time '%s'", value), args.usage = 1, args);
            long lValue = atol(value);
            if (lValue < 0 || lValue > 60) return (sprintf(args.error, "ridiculous waiting time '%s'", value), args.usage = 1, args);
            args.timeout = lValue;
        }
        else if (av[i][0] == '-')
        {
            args.usage = 1;
            sprintf(args.error, "invalid option -- '%c'", av[i][1]);
        }
        else {
            if (args.address)
                free(args.address);
            args.address = strdup(av[i]);
        }
        i++;
    }
    return args;
}

int print_usage(ParsedArgs args, char *prog)
{
    if (strlen(args.error))
    {
        printf("%s: %s\n", prog, args.error);
        printf("Try 'ft_traceroute --help' for more information.\n");
        return 64;
    }
    printf("\nUsage:  ft_traceroute [OPTION...] HOST\n");
    printf("Print the route packets trace to network host.\n\n");
    printf("  -m              set maximal hop count (default: 64)\n");
    printf("  -q              set number of probes per hop (default: 3)\n");
    printf("  -p              set starting port (default: 33434)\n");
    printf("  -w              set wait time (default: 3)\n");
    printf("  --help              give this help list\n");
    return 2;
}