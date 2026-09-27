CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: bank_client bank_server

clean:
	rm -f bank_client bank_server

bank_client: bank_client.c helpers.c helpers.h
	$(CC) $(CFLAGS) -o $@ bank_client.c helpers.c

bank_server: bank_server.c helpers.c helpers.h
	$(CC) $(CFLAGS) -o $@ bank_server.c helpers.c
