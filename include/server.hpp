#pragma once
#include "Database.hpp"
#include <arpa/inet.h>
#include <condition_variable> // std::condition_variable
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <ctpl_stl.h>
#include <ctype.h>
#include <exception>
#include <iostream>
#include <mutex> // std::mutex
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <queue> // std::queue
#include <signal.h>
#include <stdbool.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread> // std::thread
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

struct Connection
{
    int connection;
    string request;
};

class ThreadPool
{
private:
    // This condition variable is used for the threads to wait until there is work
    // to do
    condition_variable_any work_queue_condition_variable;

    // We store the threads in a vector, so we can later stop them gracefully
    vector<thread> threads;

    // Mutex to protect workQueue
    mutex work_queue_mutex; // workQueueMutex;

    // Queue of requests waiting to be processed
    queue<pair<int, string>> work_queue;

    // This will be set to true when the thread pool is shutting down. This tells
    // the threads to stop looping and finish
    bool done;

    // Function used by the threads to grab work from the queue
    void do_work();

    // item(file_descriptor, request)
    // the function does not need to return anything
    // function signature for our worker
    void process_request(const pair<int, string> item);

public:
    ThreadPool();

    // The destructor joins all the threads so the program can exit gracefully.
    // This will be executed if there is any exception (e.g. creating the threads)
    ~ThreadPool();

    // This function will be called by the server, every time there is a request
    // that needs to be processed by the thread pool
    // interface for our thread pool
    void queue_work(int fd, string& request);
};

void assign_port(string assigned_str, int& target_port, string name = "");

pair<int, vector<string>> parse_command(string cmd);

bool is_number(const string& s, bool double_flag = false);

void process_request(int id, Connection& conn);