#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "helpers.h"

#define BANK_BUFFER_LEN 4096
#define STARTING_BALANCE 1000

int process_request(int *balance, char *req_msg, char *res_msg) {
    int status = 0;

    if (strncmp(BANK_CMD_WITHDRAW, req_msg, strlen(BANK_CMD_WITHDRAW)) == 0) {
        char *amt_str = req_msg + strlen(BANK_CMD_WITHDRAW) + 1;
        int amt;
        status = sscanf(amt_str, "%d", &amt);
        if (status < 0) {
            sprintf(res_msg, "%s INVALID_FORMAT\n", BANK_CODE_ERR);
        } else if (amt > *balance) {
            sprintf(res_msg, "%s INSUFFICIENT_FUNDS\n", BANK_CODE_ERR);
        } else {
            *balance -= amt;
            sprintf(res_msg, "%s %s %d\n", BANK_CODE_OK, BANK_CMD_BALANCE, *balance);
        }
    } else if (strcmp(BANK_CMD_QUIT, req_msg) == 0) {
        *balance = -1;
        sprintf(res_msg, "%s\n", BANK_CODE_BYE);
    } else if (strcmp(BANK_CMD_BALANCE, req_msg) == 0) {
        sprintf(res_msg, "%s %s %d\n", BANK_CODE_OK, BANK_CMD_BALANCE, *balance);
    } else if (strncmp(BANK_CMD_DEPOSIT, req_msg, strlen(BANK_CMD_DEPOSIT)) == 0) {
        char *amt_str = req_msg + strlen(BANK_CMD_DEPOSIT) + 1;
        int amt;
        status = sscanf(amt_str, "%d", &amt);
        if (status < 0) {
            sprintf(res_msg, "%s INVALID_FORMAT\n", BANK_CODE_ERR);
        } else {
            *balance += amt;
            sprintf(res_msg, "%s %s %d\n", BANK_CODE_OK, BANK_CMD_BALANCE, *balance);
        }
    } else {
        sprintf(res_msg, "%s INVALID_CMD\n", BANK_CODE_ERR);
    }

    return status;
}

int accept_tcp(int sock_fd) {
    int status = NO_ERROR;
    int balance = STARTING_BALANCE;

    char msg_buffer[MSG_MAX_LEN];
    char server_response[MSG_MAX_LEN];

    status = listen(sock_fd, 1);
    if (status < 0) {
        perror("listen()\n");
        goto cleanup;
    }

    int client_fd = 0; 
    while (client_fd == 0 || errno == EINTR) {
        client_fd = accept(sock_fd, NULL, NULL);
    }
    if (client_fd < 0) {
        perror("accept()\n");
        goto cleanup;
    }

    while (1) {
        long bytes_received = 0;
        while (1) {
            status = recv(client_fd, msg_buffer + bytes_received, 1, 0);
            if (status < 0 && errno != EINTR) {
                perror("recv()\n");
                goto cleanup;
            } else if (status == 0) {
                status = ERROR;
                goto cleanup;
            } else if (status > 0) {
                if (msg_buffer[bytes_received++] == '\n') {
                    break;
                }
            }
        }
        msg_buffer[bytes_received - 1] = '\0';

        status = process_request(&balance, msg_buffer, server_response);
        if (status < 0) {
            perror("process_request()\n");
        }
        
        long bytes_sent = 0;
        int msg_len = strlen(server_response);
        while (bytes_sent < msg_len) {
            status = send(client_fd, server_response + bytes_sent, msg_len, 0);
            if (status < 0 && errno != EINTR) {
                perror("send()\n");
                goto cleanup;
            } else {
                bytes_sent += status;
            }
        }

        if (balance == -1) {
            // QUIT command
            break;
        }
    }
    
cleanup:
    close_cleanly(&client_fd);

    return status;
}

int accept_udp(int sock_fd) {
    int status = NO_ERROR;
    int balance = STARTING_BALANCE;

    char msg_buffer[MSG_MAX_LEN];
    char server_response[MSG_MAX_LEN];

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
        long bytes_received = 0;
        while (1) {
            status = recvfrom(sock_fd, msg_buffer, MSG_MAX_LEN, 0, (struct sockaddr *)&client_addr, &client_addr_len);
            if (status < 0 && errno != EINTR) {
                perror("recv()\n");
                goto cleanup;
            } else if (status >= 0) {
                bytes_received = status;
                break;
            }
        }
        msg_buffer[bytes_received - 1] = '\0';

        status = process_request(&balance, msg_buffer, server_response);
        if (status < 0) {
            perror("process_request()\n");
        }
        
        long bytes_sent = 0;
        int msg_len = strlen(server_response);
        while (bytes_sent < msg_len) {
            status = sendto(sock_fd, server_response, msg_len, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            if (status < 0 && errno != EINTR) {
                perror("send()\n");
                goto cleanup;
            } else {
                bytes_sent += status;
            }
        }

        if (balance == -1) {
            // QUIT command
            break;
        }
    }
    
cleanup:
    return status;
}

int main(int argc, char** argv) {
    int status = NO_ERROR;

    const char help_msg[] = "Usage: ./bank_server <tcp|udp> <port>\n";

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

    struct sockaddr_in sock_addr = get_server_sock(args.port, 1);
    status = bind(sock_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (status < 0) {
        perror("bind()\n");
        goto cleanup;
    }

    if (args.sock_type == SOCK_STREAM) {
        accept_tcp(sock_fd);
    } else if (args.sock_type == SOCK_DGRAM) {
        accept_udp(sock_fd);
    } else {
        printf("Invalid socket type (logic error).\n");
        goto cleanup;
    }

cleanup:
    close_cleanly(&sock_fd);

    return status;
}