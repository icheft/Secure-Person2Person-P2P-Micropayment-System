#pragma once
#include <netinet/in.h>
#include <stdio.h>

const char* man = "Usage: ./client <IP_address> <port_number>\n"
                  "By default, IP_address should be 127.0.0.1;\n"
                  "port_number should be an integer between 1024 and 65535.";

void sending();
void receiving(int server_fd); // taking in file descriptor
void* receive_thread(void* server_fd);
void get_info(struct sockaddr_in*);