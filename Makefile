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

SRCDIR = src

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CFLAGS=-lstdc++ -lpthread -lsqlite3 $(INCLUDE)
endif

ifeq ($(UNAME), Linux)
CFLAGS=-lstdc++ -pthread -lrt -lpthread -lsqlite3 $(INCLUDE)
endif


.PHONY: all clean

	
all: client server
	@$(ECHO) All done
	@echo 🍺 Type \"./server \<PORT\> \<LIMIT\>\" to start the server;
	@echo 🍺 Type \"./client \<SERVER_IP\> \<SERVER_PORT\>\" to start the client.
	@echo 💬 \(Or type \"make clean\" to clean the binaries\)


# FIXME - bad name here
client: src/client.cpp
	@$(ECHO) Compiling $@
	@$(CC) $(CLIB) $(WORKDIR)/$(SRCDIR)/client.cpp $(CFLAGS) -o client

# FIXME - bad name here
server: src/server.cpp src/Database.cpp
	@$(ECHO) Compiling $@
	@$(CC) $(CLIB) $(WORKDIR)/$(SRCDIR)/server.cpp $(WORKDIR)/$(SRCDIR)/Database.cpp $(CFLAGS) -o server


testing: test/db-test.cpp
	@echo $@

clean: 
	@rm -f client server
	@echo 🧹 client and server binaries are now removed.

# TODO: database opt
# clean-all: 
# 	rm -f client server server.db
# 	@echo 🧹 client and server binaries are now removed.

endif