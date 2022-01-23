#pragma once
#include <sys/socket.h>
#ifdef __APPLE__
// #define sth for apple
#define RECV_SIGNAL SO_NOSIGPIPE
#elif __linux
// linux
#define RECV_SIGNAL MSG_NOSIGNAL
#elif __unix // all unices not caught above
// Unix
#define RECV_SIGNAL 0
#elif __posix
// POSIX
#define RECV_SIGNAL 0
#endif

#include "Database.hpp"
#include <arpa/inet.h>
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <ctpl_stl.h>
#include <ctype.h>
#include <exception>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <signal.h> // sigaction
#include <stdbool.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
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
// #define LIMIT 12 // TODO: change limit dynamically

const char* man = "Usage: ./server <port_number> <thread_limit> [-s]\n"
                  "port_number should be an integer between 1024 and 65535.\n"
                  "thread_limit determines the number of users that can connect.\n"
                  "If [-s] or [--silent] is specified, silent mode will be enabled.\n";

vector<string> split(string str, string sep);

void sigint_handler(sig_atomic_t s);
void sigpipe_handler(int unused);

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

tuple<bool, bool> check_db_status();
