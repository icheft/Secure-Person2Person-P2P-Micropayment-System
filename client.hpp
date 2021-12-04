#pragma once
#include <netinet/in.h>
#include <stdio.h>
#include <string>
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

int receiving(int socket_fd); // taking in file descriptor
void* receive_thread(void* socket_fd);
void get_info(struct sockaddr_in*);
// int check_command(char* msg);

char* exit_server(int socket_fd);
char* register_user(int socket_fd);
char* login_server(int socket_fd, int* login_port);
char* request_list(int socket_fd);
char* p2p_transaction(int socket_fd);
vector<string> find_peer_info(char* name);
void parse_list_info(char* msg);
void print_sys_info();

vector<string> split(string str, string sep);