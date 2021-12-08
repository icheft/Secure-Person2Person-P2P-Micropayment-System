CC=gcc

CLIB=-std=c++11

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CFLAGS=-lstdc++ -lpthread
endif

ifeq ($(UNAME), Linux)
CFLAGS=-lstdc++ -pthread -lrt -lpthread
endif

client: client.cpp client.hpp
	$(CC) $(CLIB) ./client.cpp $(CFLAGS) -o client
bt: test/client_bt.c
	$(CC) ./client_bt.c $(CFLAGS) -o bt
p2p: test/client_p2p.cpp
	$(CC) $(CLIB) ./test/client_p2p.cpp $(CFLAGS) -o p2p
mic: test/mic.cpp
	$(CC) $(CLIB) ./test/mic.cpp $(CFLAGS) -o mic
host: host.cpp
	$(CC) $(CLIB) ./host.cpp $(CFLAGS) -o host


.PHONY: clean

clean: 
	rm -f client
