workdir:="$(shell pwd)"

CC=gcc

CLIB=-std=c++17

INCLUDE = -I$(workdir)/include/\
                    -I$(workdir)/src/

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CFLAGS=-lstdc++ -lpthread -lsqlite3 $(INCLUDE)
endif

ifeq ($(UNAME), Linux)
CFLAGS=-lstdc++ -pthread -lrt -lpthread -lsqlite3 $(INCLUDE)
endif

run: client
	@echo 🍺 Type \"./client \<SERVER_IP\> \<SERVER_PORT\>\" to start the client.
	@echo 💬 \(Or type \"make clean\" to clean the binaries\)
	
# run: client server
# 	@echo 🍺 Type \"./server \<PORT\> -\<OPTION\>\" to start the server;
# 	@echo 🍺 Type \"./client \<SERVER_IP\> \<SERVER_PORT\>\" to start the client.
# 	@echo 💬 \(Or type \"make clean\" to clean the binaries\)


# FIXME - bad name here
client: src/client.cpp
	$(CC) $(CLIB) $(workdir)/src/client.cpp $(CFLAGS) -o client

# FIXME - bad name here
server: src/server.cpp src/Database.cpp
	$(CC) $(CLIB) $(workdir)/src/server.cpp $(workdir)/src/Database.cpp $(CFLAGS) -o server

testing: test/db-test.cpp
	$(CC) $(CLIB) $(workdir)/test/db-test.cpp $(CFLAGS) -o $(workdir)/test/db-test.out

.PHONY: clean

clean: 
	rm -f client server
	@echo 🧹 client and server binaries are now removed.

clean-all: 
	rm -f client server server.db
	@echo 🧹 client, server, and serber database are now removed.
