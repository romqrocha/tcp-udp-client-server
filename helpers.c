#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "helpers.h"

Args parse_args(int argc, char** argv, int *success) {
    *success = ERROR;
    argc -= 1; // ignore program name argument
    Args args = {0};

    const char *arg_count_err = "Error: This program takes in %d arguments (not %d).\n";
    const char *bad_proto_err = "Error: You must enter 'udp' or 'tcp' (case sensitive)" \
                                "for the protocol argument (not %s).\n";
    const char *bad_port_err = "Error: Port argument must be a number between %d-%d.\n";

    if (argc != ARG_COUNT) {
        printf(arg_count_err, ARG_COUNT, argc);
        return args;
    }

    if (strstr(argv[1], "tcp") == argv[1]) {
        args.sock_type = SOCK_STREAM;
    } else if (strstr(argv[1], "udp") == argv[1]) {
        args.sock_type = SOCK_DGRAM;
    } else {
        printf(bad_proto_err, argv[1]);
    }

    int port;
    int scans = sscanf(argv[2], "%d", &port);
    if (scans != 1 || port < MIN_PORT || port > MAX_PORT) {
        printf(bad_port_err, MIN_PORT, MAX_PORT);
        return args;
    }
    args.port = (port_t)port;

    *success = NO_ERROR;
    return args;
}

struct sockaddr_in get_server_sock(port_t port, int any_ip) {
    struct sockaddr_in sock_addr = {
        .sin_family = AF_INET, 
        .sin_port = htons(port), 
        .sin_addr = {.s_addr=any_ip ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK)}
    };
    return sock_addr;
}

void close_cleanly(int *fd) {
    while (*fd) { // continue only if fd != 0
        if (close(*fd) < 0) { // if error
            if (errno != EINTR) { // if error wasn't a sys interrupt
                perror("close()\n");
                break;
            }
        } else { // closed successfully
            *fd = 0;
        }
    }
}