#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>

#define PORT 43982
#define MAX_CONNECTIONS 16
#define MSG_LEN 32

int main() {
    int status = 0;
    
    int client_fd = 0;
    struct sockaddr_in server_addr = {
        .sin_family=AF_INET, 
        .sin_port=PORT, 
        .sin_addr=INADDR_LOOPBACK
    };

    long bytes_sent = 0;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket()\n");
        goto cleanup;
    }


    status = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status < 0) {
        perror("connect()\n");
        goto cleanup;
    }

    char msg[MSG_LEN] = "Hello world! (1)";
    size_t msg_len = strlen(msg_len);

    while (bytes_sent < msg_len) {
        status = send(client_fd, msg, msg_len, 0);
        if (status < 0) {
            perror("send()\n");
            goto cleanup;
        } else {
            bytes_sent += status;
        }
    }

cleanup:
    if (client_fd) {
        shutdown(client_fd, SHUT_WR);
        close(client_fd);
    }
    return 0;
}