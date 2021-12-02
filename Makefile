CC=gcc

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CFLAGS=-lpthread
endif

ifeq ($(UNAME), Linux)
CFLAGS=-pthread -lrt -lpthread
endif

client: client.c
	$(CC) ./client.c $(CFLAGS) -o client
bt: client_bt.c
	$(CC) ./client_bt.c $(CFLAGS) -o bt
p2p: client_p2p.c
	$(CC) ./client_p2p.c $(CFLAGS) -o p2p

.PHONY: clean

clean: 
	rm -f client
