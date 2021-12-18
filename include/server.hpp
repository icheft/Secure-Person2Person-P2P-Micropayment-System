#pragma once
#include "Database.hpp"
#include <arpa/inet.h>
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <ctpl_stl.h>
#include <ctype.h>
#include <exception>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
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
#define LIMIT 12

vector<string> split(string str, string sep);

void sigint_handler(sig_atomic_t s);

// struc to be pass into threadpool
struct Connection
{
    int connection;
    string request;
};

void assign_port(string assigned_str, int& target_port, string name = "");

pair<int, vector<string>> parse_command(string cmd);

bool is_number(const string& s, bool double_flag = false);

void process_request(int id, Connection& conn);