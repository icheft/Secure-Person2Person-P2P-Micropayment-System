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

.PHONY: clean

clean: 
	rm -f client
