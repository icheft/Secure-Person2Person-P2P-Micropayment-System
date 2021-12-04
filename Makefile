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
p2p: test/client_p2p.c
	$(CC) ./test/client_p2p.c $(CFLAGS) -o p2p
mic: test/mic.cpp
	$(CC) $(CLIB) ./test/mic.cpp $(CFLAGS) -o mic


.PHONY: clean

clean: 
	rm -f client
