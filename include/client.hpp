#pragma once
#include <arpa/inet.h>
#include <ctype.h>
#include <exception>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <vector>
using namespace std;
#define REGISTER 1
#define LOGIN 2
#define LIST 3
#define TRANSACTION 4
#define EXIT 5
#define ERR -1

const char* man = "Usage: ./client <IP_address> <port_number>\n"
                  "By default, IP_address should be 127.0.0.1;\n"
                  "port_number should be an integer between 1024 and 65535.";

const char* notice = "Please specify an IP address and a port number to connect to.";

const char* default_program_msg = "Now running on default...";

// utility

void sigint_handler(sig_atomic_t s);

// threads

void* receive_thread(void* socket_fd);
int receiving(int socket_fd); // taking in file descriptor
void get_info(struct sockaddr_in*);

// client functions
char* exit_server(int socket_fd);
char* register_user(int socket_fd);
char* login_server(int socket_fd, int* login_port);
char* request_list(int socket_fd);
char* p2p_transaction(int socket_fd);

// self-defined
vector<string> find_peer_info(char* name);
void parse_list_info(char* msg);
void print_sys_info();

vector<string> split(string str, string sep);