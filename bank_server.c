#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>

#define PORT 43982
#define MAX_CONNECTIONS 16
#define BUFFER_LEN 4096
#define TCP 1

int talk_to_client(int client_fd, struct sockaddr_in client_addr, socklen_t client_addr_len) {
    int status = 0;

    size_t bytes_received = 0;

    char *buffer = calloc(BUFFER_LEN, sizeof(char));

    while (bytes_received < BUFFER_LEN) {
            status = recv(client_fd, buffer, BUFFER_LEN - bytes_received, 0);
        if (status < 0) {
            perror("recv()\n");
            goto cleanup;
        } else if (status == 0) {
            printf("Connection terminated.\n"); // TCP
            break;
        } else {
            bytes_received += status;
        }
    }
    
cleanup:
    free(buffer);

    return 0;
}

int main() {
    int status = 0;

    int fd = 0;
    int client_fd[MAX_CONNECTIONS] = {0};
    
    struct sockaddr_in addr = {
        .sin_family=AF_INET, 
        .sin_port=PORT, 
        .sin_addr=INADDR_LOOPBACK
    };
    struct sockaddr_in client_addr[MAX_CONNECTIONS] = {0};
    socklen_t client_addr_len[MAX_CONNECTIONS] = {0};
    
    fd = socket(AF_INET, SOCK_STREAM, 0); // UDP = SOCK_DGRAM, TCP = SOCK_STREAM;
    if (fd < 0) {
        perror("socket()\n");
        goto cleanup;
    }

    
    status = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0) {
        perror("bind()\n");
        goto cleanup;
    }

    status = listen(fd, MAX_CONNECTIONS);
    if (status < 0) {
        perror("listen()\n");
        goto cleanup;
    }

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        client_fd[i] = accept(fd, (struct sockaddr *)&(client_addr[i]), &(client_addr_len[i]));
        if (client_fd[i] < 0) {
            perror("accept()\n");
            goto cleanup;
        }

        int cpid = fork();
        if (cpid < 0) {
            perror("fork()\n");
            goto cleanup;
        } else if (cpid == 0) {
            status = talk_to_client(client_fd[i], client_addr[i], client_addr_len[i]);
            if (status < 0) {
                perror("talk_to_client()\n");
                exit(1);
            } else {
                exit(0);
            }
        }
    }

cleanup:
    if (fd) {
        close(fd);
    }
    if (client_fd) {
        close(client_fd);
    }

    return 0;
}