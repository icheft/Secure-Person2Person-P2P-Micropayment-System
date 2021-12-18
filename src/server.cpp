#include "server.hpp"

// global variables for signal handler
Database* db;
ThreadPool* tp;
int socket_fd;
bool termination_flag = false; // be careful of what you wish for

const int connection_limit = 10;
string public_key = "public_key";
vector<pair<string, int*>> client_fds;

// This class manages a thread pool that will **process requests**
void ThreadPool::do_work()
{
    // Loop while the queue is not destructing
    while (true && !termination_flag) { //! this->done
        pair<int, string> request;

        // Create a scope, so we don't lock the queue for longer than necessary
        {
            unique_lock<mutex> g(this->work_queue_mutex);
            this->work_queue_condition_variable.wait(g, [&] {
                // & introduces a lambda expression. The contents of the square brackets indicate what is to be captured inside the lambda. Having only a & in there means that everything that is mentioned inside the lambda and can be found outside of its scope is captured by reference.
                // only wake up if there are elements in the queue or the program is
                // shutting down
                // i.e. will return true if the program is working
                return !this->work_queue.empty() || this->done;
            });
            if (this->done && this->work_queue.empty()) {
                // lock will be release automatically.
                // finish the thread loop and let it join in the main thread.
                return;
            }

            request = this->work_queue.front();
            this->work_queue.pop();
        }
        this->process_request(request);
    }
}

void ThreadPool::process_request(const pair<int, string> item)
{
    // handle the cases where we are terminating the server
    // exit if not init item
    if (item.second != "init") {
        return;
    }

    // after the connection is given
    int connection = item.first;

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(connection, (struct sockaddr*)&addr, &addr_size);
    char* clientip = new char[20];
    int clientport;
    strcpy(clientip, inet_ntoa(addr.sin_addr));
    clientport = ntohs(addr.sin_port);

    char display_msg[1024];

    sprintf(display_msg, "→ Accepting new connection from:\n"
                         "  Client IP: %s\n"
                         "  Client Port: %d\n",
        clientip, clientport);
    printf("%s\n", display_msg);
    string username = "";

    while (true && connection && !termination_flag) {
        // Read from the connection
        char buffer[2048];
        int tmp_byte_read = recv(connection, buffer, sizeof(buffer), 0);
        // string request = buffer ;
        string raw(buffer);

        sprintf(display_msg, "→ [%s] from %s@%d\n", raw.c_str(), clientip, clientport);

        printf("%s\n", display_msg);

        // TODO: server functions

        pair<int, vector<string>>
            item = parse_command(raw);
        int cmd_type = item.first;
        vector<string> processed_cmd = item.second;

        string response = "";
        // REGISTER 1
        // LOGIN 2
        // LIST 3
        // TRANSACTION 4
        // EXIT 5
        // ERR -1
        switch (cmd_type) {
        case REGISTER: {
            int status = db->user_register(processed_cmd[1], clientip);
            if (status == REGISTER_OK)
                response += to_string(status) + " " + "OK\n";
            else
                response += to_string(status) + " " + "FAIL\n";

            send(connection, response.c_str(), response.size(), 0);
            break;
        }
        case LOGIN: {
            int status = db->user_login(processed_cmd[0], clientip, clientport, stoi(processed_cmd[1]), connection);

            if (status == LOGIN_AUTH_FAIL)
                response += to_string(status) + " " + "AUTH_FAIL\n";
            else if (status == LOGIN_EXIST) {
                response += to_string(LOGIN_EXIST) + " " + "DUPLICATE_LOGIN\n";
            } else if (status == LOGIN_SUCCESS) {
                username = processed_cmd[0];
                client_fds.push_back(make_pair(username, &connection));
                auto user = db->user_info(username);
                response = to_string(user.balance) + "\n";
                response += public_key + "\n";
                response += to_string(db->user_num()) + "\n";
                printf("Online num: %d\n", db->user_num());
                response += db->user_list_info();
            }
            send(connection, response.c_str(), response.size(), 0);
            break;
        }
        case LIST: {
            auto online_users = db->list(); // online is true by default
            if (online_users.size() == 0 || username.empty()) {
                response += "Please login first\n";
            } else {
                auto user = db->user_info(username);
                response = to_string(user.balance) + "\n";
                response += public_key + "\n";
                response += to_string(db->user_num()) + "\n";
                response += db->user_list_info();
            }
            send(connection, response.c_str(), response.size(), 0);
            break;
        }
        case TRANSACTION: {
            int status = db->user_transaction(processed_cmd[0], processed_cmd[2], stod(processed_cmd[1]));
            if (status == TRANSFER_OK)
                response = "transfer OK\n";
            else if (status == TRANSFER_FAIL)
                response = "transfer FAIL\n";
            else if (status == TRANSFER_SENDER_BANKRUPT)
                response = "transfer FAIL due to bankruptcy\n";

            // to the sender
            auto sender = db->user_info(processed_cmd[0]);
            int* sender_fd;
            for (int i = 0; i < client_fds.size(); i++) {
                if (client_fds[i].first == sender.username) {
                    // NOTE: have to use global file descriptor to get the job done
                    sender_fd = client_fds[i].second;
                    break;
                }
            }
            // cout << "sender fd: " << sender.fd << " vs. " << *sender_fd << endl;
            send(*sender_fd, response.c_str(), response.size(), 0);
            break;
        }
        case EXIT: {
            int status = db->user_logout(clientip, clientport);
            response += "Bye\n";
            send(connection, response.c_str(), response.size(), 0);
            int online_num = db->user_num();
            for (int i = 0; i < client_fds.size(); i++) {
                if (client_fds[i].first == username) {
                    client_fds.erase(client_fds.begin() + i);
                    break;
                }
            }
            // close connection with this user
            // Close the connection
            printf("Online num:%d\n", online_num);
            close(connection);

            shutdown(connection, SHUT_RDWR); // FIXME: THE TIME_WAIT thing: tcp4       0      0  127.0.0.1.64480                               127.0.0.1.8888                                TIME_WAIT
            // REF: https://stackoverflow.com/questions/23915304/how-to-avoid-time-wait-for-server-sockets
            break;
        }
        default: {
            response = to_string(QUERY_ERROR);
            send(connection, response.c_str(), response.size(), 0);
            break;
        }
        }

        if (cmd_type == EXIT) {
            break;
        }
    }
    return;
}

ThreadPool::ThreadPool()
    : done(false)
{
    // This returns the number of threads supported by the system. If the
    // function can't figure out this information, it returns 0. 0 is not good,
    // so we create at least 1
    // [note]: every time we receive a request, we will send the request to our thread pool
    auto num_of_threads = thread::hardware_concurrency();
    if (num_of_threads == 0) {
        num_of_threads = 1;
    }

    // cout << "Number of threads: " << num_of_threads << endl;

    for (unsigned i = 0; i < num_of_threads; i++) {
        // The threads will execute the private member `do_work`. Note that we need
        // to pass a reference to the function (namespaced with the class name) as
        // the first argument, and the current object as second argument
        this->threads.push_back(thread(&ThreadPool::do_work, this));
    }
}

ThreadPool::~ThreadPool()
{
    unique_lock<mutex> lock(work_queue_mutex);
    // So threads know it's time to shut down
    this->done = true;
    lock.unlock();

    // Wake up all the threads, so they can finish and be joined
    this->work_queue_condition_variable.notify_all();
    int k = 0;
    for (auto& thread : this->threads) {
        // FIXME - thread cannot be closed
        // cout << ++k << endl;
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ThreadPool::queue_work(int fd, string& request)
{
    {
        // Grab the mutex - lock_guard
        unique_lock<mutex> g(this->work_queue_mutex);

        // Push the request to the queue
        this->work_queue.push(pair<int, string>(fd, request));
    }

    // Notify one thread that there are requests to process
    this->work_queue_condition_variable.notify_one();
}

int main(int argc, char const* argv[])
{
    // handler
    // signal(SIGINT, sigint_handler);

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf("Failed to caught signal\n");
    }

    // Create a socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create a socket. Now aborting.");
        exit(EXIT_FAILURE);
    }

    int server_port;

    switch (argc) {
    case 2: {
        assign_port(string(argv[1]), server_port);
        break;
    }
    default: {
        // using default
        printf("Probing for an available port...\n");
        // default
        assign_port("0", server_port);
        break;
    }
    }

    cout << "Server listening on port " << server_port << endl;

    // Listen to a port
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    sockaddr.sin_port = htons(server_port);

    if (::bind(socket_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        cout << "Failed to bind to port " << server_port << ". errno: " << errno << endl;
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(socket_fd, 5) < 0) {
        cout << "Failed to listen on socket. errno: " << errno << endl;
        exit(EXIT_FAILURE);
    }

    // server database
    db = new Database(true, true);
    tp = new ThreadPool;

    while (true) {
        // Grab a connection from the queue
        auto addrlen = sizeof(sockaddr);
        int connection = accept(socket_fd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
        if (connection < 0) {
            cout << "Failed to grab connection. errno: " << errno << endl;
            exit(EXIT_FAILURE);
        } else {
            // TODO connection limit
            // pass
        }

        string request = "init";

        // Add some work to the queue
        tp->queue_work(connection, request);
    }

    close(socket_fd);

    return 0;
}

void assign_port(string assigned_str, int& target_port, string name)
{
    if (stoi(assigned_str) == 0) {
        struct sockaddr_in tmp_sin;
        int tmp_socket;

        tmp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tmp_socket == -1) {
            perror("Unknown error occurred. Failed to create a socket. Now aborting.");
            exit(EXIT_FAILURE);
        }

        tmp_sin.sin_port = htons(0);
        tmp_sin.sin_addr.s_addr = INADDR_ANY;
        tmp_sin.sin_family = AF_INET;

        if (::bind(tmp_socket, (struct sockaddr*)&tmp_sin, sizeof(struct sockaddr_in)) == -1) {
            perror("Unable to locate a free port. Now aborting.");
            exit(EXIT_FAILURE);
        }

        socklen_t len = sizeof(tmp_sin);

        if (getsockname(tmp_socket, (struct sockaddr*)&tmp_sin, &len) != -1) {
            target_port = ntohs(tmp_sin.sin_port);
            if (!name.empty())
                printf("Requesting %s on port # %d...\n", name.c_str(), target_port);
            else
                printf("Requesting port # %d...\n", target_port);
        }
        close(tmp_socket);
    } else {
        target_port = stoi(assigned_str);
    }
}

void sigint_handler(sig_atomic_t s)
{
    printf("Caught signal %d\n", s);
    close(socket_fd);
    shutdown(socket_fd, SHUT_RDWR);
    for (auto& fd : client_fds) {
        close(*fd.second);
        shutdown(*fd.second, SHUT_RDWR);
    }

    termination_flag = true; // danger
    // TODO: delete all thread
    delete tp;
    delete db;
    exit(1);
}

vector<string> split(string str, string sep)
{
    char* cstr = const_cast<char*>(str.c_str());
    char* current;
    vector<string> arr;
    current = strtok(cstr, sep.c_str());
    while (current != NULL) {
        arr.push_back(current);
        current = strtok(NULL, sep.c_str());
    }
    return arr;
}

bool is_number(const string& s, bool double_flag)
{
    if (!double_flag)
        return (strspn(s.c_str(), "0123456789") == s.size());
    else {
        return (strspn(s.c_str(), "-.0123456789") == s.size());
    }
}

pair<int, vector<string>> parse_command(string cmd)
{
    vector<string> args = split(cmd, "#");
    /*
    REGISTER 1
    LOGIN 2
    LIST 3
    TRANSACTION 4
    EXIT 5
    ERR -1
    */
    int cmd_type;

    if (args.size() == 1 && args[0] == "Exit") {
        cmd_type = EXIT;
    } else if (args.size() == 1 && args[0] == "List") {
        cmd_type = LIST; // Please login first\n
    } else if (args.size() == 2 && args[0] == "REGISTER") {
        cmd_type = REGISTER;
    } else if (args.size() == 2 && is_number(args[1])) {
        cmd_type = LOGIN;
    } else if (args.size() == 3 && is_number(args[1], true)) {
        cmd_type = TRANSACTION;
    } else
        cmd_type = ERR;

    return make_pair(cmd_type, args);
}