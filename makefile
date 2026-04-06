PORT ?= 54749
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -DPORT=$(PORT)

.PHONY: all clean

all: client server

client: client.o
	$(CC) $(CFLAGS) -o client client.o

server: server.o
	$(CC) $(CFLAGS) -o server server.o

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f client server client.o server.o