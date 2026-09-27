#ifndef HELPERS_H
#define HELPERS_H

#define NO_ERROR 0
#define ERROR -1

#define MSG_MAX_LEN 256

#define BANK_CODE_OK "OK"
#define BANK_CODE_ERR "ERR"
#define BANK_CODE_BYE "BYE"
#define BANK_CMD_BALANCE "BALANCE"
#define BANK_CMD_DEPOSIT "DEPOSIT"
#define BANK_CMD_WITHDRAW "WITHDRAW"
#define BANK_CMD_QUIT "QUIT"

typedef unsigned short port_t;
typedef unsigned char byte;

#define UDP 0
#define TCP 1
#define MIN_PORT 1024
#define MAX_PORT 65535
#define ARG_COUNT 2
typedef struct Args {
    int sock_type;
    port_t port;
} Args;

Args parse_args(int argc, char** argv, int *success);
struct sockaddr_in get_server_sock(port_t port, int any_ip);
void close_cleanly(int *fd);

#endif