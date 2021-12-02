#pragma once
#include <stdio.h>

void sending();
void receiving(int server_fd); // taking in file descriptor
void* receive_thread(void* server_fd);