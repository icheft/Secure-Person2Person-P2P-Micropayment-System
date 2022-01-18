ifneq ($(words $(MAKECMDGOALS)),1) # if no argument was given to make...
.DEFAULT_GOAL = all # set the default goal to all
%:                   # define a last resort default rule
	@$(MAKE) $@ --no-print-directory -rRf $(firstword $(MAKEFILE_LIST)) # recursive make call, 
else
ifndef ECHO
T := $(shell $(MAKE) $(MAKECMDGOALS) --no-print-directory \
      -nrRf $(firstword $(MAKEFILE_LIST)) \
      ECHO="COUNTTHIS" | grep -c "COUNTTHIS")
N := x
C = $(words $N)$(eval N := x $N)
ECHO = echo -ne "\r [`expr $C '*' 100 / $T`%]"
endif

# ... cmd 


WORKDIR:="$(shell pwd)"

CC=gcc

CLIB=-std=c++17

INCLUDE = -I$(WORKDIR)/include/\
                    -I$(WORKDIR)/src/					

OPENSSLCPPFLAGS = -I/usr/local/opt/openssl@1.1/include

LDFLAGS = -L/usr/local/opt/openssl@1.1/lib

SRCDIR = src

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CFLAGS=-lstdc++ -lpthread -lsqlite3 $(LDFLAGS) $(INCLUDE) $(OPENSSLCPPFLAGS) -lssl -lcrypto 
endif

ifeq ($(UNAME), Linux)
CFLAGS=-lstdc++ -pthread -lrt -lpthread -lsqlite3 $(INCLUDE) -lssl -lcrypto
endif


.PHONY: all clean

	
all: client server
	@$(ECHO) All done
	@echo 🍺 Type \"./server \<PORT\> \<LIMIT\> \[-s\]\" to start the server;
	@echo 🍺 Type \"./client \<SERVER_IP\> \<SERVER_PORT\> \[-v\]\" to start the client.
	@echo 💬 \(Or type \"make clean\" to clean the binaries\)


# FIXME - bad name here
client: src/client.cpp include/util.hpp
	@$(ECHO) Compiling $@
	@$(CC) $(CLIB) $(WORKDIR)/$(SRCDIR)/client.cpp $(CFLAGS) -o client

# FIXME - bad name here
server: src/server.cpp src/Database.cpp include/util.hpp
	@$(ECHO) Compiling $@
	@$(CC) $(CLIB) $(WORKDIR)/$(SRCDIR)/server.cpp $(WORKDIR)/$(SRCDIR)/Database.cpp $(CFLAGS) -o server

debug: src/client.cpp src/server.cpp src/Database.cpp include/util.hpp
	@$(ECHO) Compiling $@
	@$(CC) $(CLIB) -DDEBUG $(WORKDIR)/$(SRCDIR)/client.cpp $(CFLAGS) -o client_d
	@$(CC) $(CLIB) -DDEBUG $(WORKDIR)/$(SRCDIR)/server.cpp $(WORKDIR)/$(SRCDIR)/Database.cpp $(CFLAGS) -o server_d
	@echo 🍺 Type \"./server_d \<PORT\> \<LIMIT\> \[-s\]\" to start the server;
	@echo 🍺 Type \"./client_d \<SERVER_IP\> \<SERVER_PORT\> \[-v\]\" to start the client.
	@echo 💬 \(Or type \"make clean\" to clean the binaries\)

testing: test/write_crt.cpp
	@$(CC) $(CLIB) test/write_crt.cpp $(CFLAGS) -o wc

simple: src/simple.cpp
	@$(CC) $(CLIB) src/simple.cpp $(CFLAGS) -o simple


clean: 
	@rm -f client server
	@echo 🧹 client and server binaries are now removed.

# TODO: database opt
# clean-all: 
# 	rm -f client server server.db
# 	@echo 🧹 client and server binaries are now removed.

endif