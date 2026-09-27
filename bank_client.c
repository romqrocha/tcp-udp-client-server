#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>

#include "helpers.h"

#define SERVER_CLOSED_ERR "Error: Server closed connection unexpectedly.\n"
#define MSG_OVERFLOW_ERR "Error: Message received was longer than the max length.\n"
#define BYE_MSG "'BYE' received. Cleanly ending the connection.\n"

int connection_loop(int sock_fd, port_t server_port, int is_tcp) {
    int status = NO_ERROR;

    struct sockaddr_in server_addr = get_server_sock(server_port, 0);
    if (is_tcp) {
        status = connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (status < 0) {
            perror("connect()\n");
            goto cleanup;
        }
    }

    char user_input[MSG_MAX_LEN];
    char server_output[MSG_MAX_LEN];
    while (1) {        
        fgets(user_input, MSG_MAX_LEN, stdin);
        int msg_len = strlen(user_input);

        if (msg_len == 0) {
            break;
        }

        long bytes_sent = 0;
        while (bytes_sent < msg_len) {
            struct sockaddr *sockaddr_server_addr = (is_tcp ? NULL : (struct sockaddr *)&server_addr);
            socklen_t len = (is_tcp ? 0 : sizeof(server_addr));
            status = sendto(sock_fd, user_input + bytes_sent, msg_len, 0, sockaddr_server_addr, len);
            if (status < 0 && errno != EINTR) {
                perror("send()\n");
                goto cleanup;
            } else {
                bytes_sent += status;
            }
        }

        long bytes_received = 0;
        size_t recv_size = is_tcp ? 1 : MSG_MAX_LEN;
        while (1) {
            status = recv(sock_fd, server_output + bytes_received, recv_size, 0);
            if (status < 0 && errno != EINTR) {
                perror("recv()\n");
                goto cleanup;
            } else if (status == 0 && is_tcp) {
                status = ERROR;
                printf(SERVER_CLOSED_ERR);
                goto cleanup;
            } else if (status >= MSG_MAX_LEN) {
                status = ERROR;
                printf(MSG_OVERFLOW_ERR);
                goto cleanup;
            } else if (status > 0) {
                if (server_output[bytes_received + status - 1] == '\n') {
                    bytes_received += status;
                    break;
                }
                bytes_received += status;
            }
        }
        server_output[bytes_received] = '\0';

        printf("%s", server_output);

        int reply_is_bye = strstr(server_output, BANK_CODE_BYE) == server_output; 
        if (reply_is_bye) {
            printf(BYE_MSG);
            goto cleanup;   
        }
    }

cleanup:
    return status;
}

int main(int argc, char** argv) {
    int status = NO_ERROR;

    const char help_msg[] = "Usage: ./bank_client <tcp|udp> <port>\n";
    
    Args args = parse_args(argc, argv, &status);
    if (status != NO_ERROR) {
        printf(help_msg);
        goto cleanup;
    }
    
    int sock_fd = socket(AF_INET, args.sock_type, 0);
    if (sock_fd < 0) {
        perror("socket()\n");
        goto cleanup;
    }

    connection_loop(sock_fd, args.port, args.sock_type == SOCK_STREAM);

cleanup:
    close_cleanly(&sock_fd);
    
    return status;
}