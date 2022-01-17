#include "server.hpp"
#include "util.hpp"
#define MAX_LENGTH 2048
// global variables for signal handler
Database* db;
int socket_fd;
bool termination_flag = false; // be careful of what you wish for

const int connection_limit = 10;
string public_key = "public_key";
vector<pair<string, int*>> client_fds;
vector<pair<string, SSL*>> client_SSLs;
int current_user = 0;
int tmp_current_user = 0;
bool verbose = true;

// server will not regenerate keys dynamically
string cert_path = "certs";
string target = "server";
string pem_name = "server";
string uid = "";

string key_path = cert_path + "/" + target + ".key"; //"certs/server.key";
string crt_path = cert_path + "/" + target + ".crt"; // "certs/server.crt";
string ca_path = cert_path + "/CA.pem"; // "certs/CA.pem";

SSL_CTX* ctx;

int main(int argc, char const* argv[])
{
    // handler
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf("Failed to caught signal\n");
    }

    struct sigaction sg;
    sg.sa_handler = sigpipe_handler;
    sigemptyset(&sg.sa_mask);
    sg.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sg, NULL);

    // Thread pool

    auto num_of_threads = thread::hardware_concurrency();
    if (num_of_threads == 0) {
        num_of_threads = 1;
    }

    // wait for user input, then determine how many number of threads to create

    // Create a socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create a socket. Now aborting.");
        exit(EXIT_FAILURE);
    }

    int server_port;
    int LIMIT = num_of_threads;

    switch (argc) {
    case 2: {
        assign_port(string(argv[1]), server_port);
        printf("User limit is now set to default: %d\n", LIMIT);
        break;
    }
    case 3: {
        assign_port(string(argv[1]), server_port);
        if (strcmp(argv[2], "-s") == 0 || strcmp(argv[2], "--silent") == 0) {
            verbose = false;
            printf("User limit is now set to default: %d\n", LIMIT);
        } else {
            LIMIT = atoi(argv[2]) <= num_of_threads ? atoi(argv[2]) : num_of_threads;
            printf("User limit is now set to %d\n", LIMIT);
        }
        break;
    }
    case 4: {
        if (strcmp(argv[3], "-s") == 0 || strcmp(argv[3], "--silent") == 0) {
            verbose = false;
            assign_port(string(argv[1]), server_port);
            LIMIT = atoi(argv[2]) <= num_of_threads ? atoi(argv[2]) : num_of_threads;
            printf("User limit is now set to %d\n", LIMIT);
        } else {
            printf("%s\n", man);
        }
        break;
    }
    default: {
        // using default
        printf("%s\n", man);
        printf("Probing for an available port...\n");
        // default
        assign_port("0", server_port);
        printf("User limit is now set to default: %d\n", LIMIT);
        break;
    }
    }

    bool erase, reset;
    tie(erase, reset) = check_db_status();

    ctpl::thread_pool thread_pool(LIMIT);

    // openssl

    ctx = SSL_CTX_new(SSLv23_method());

    // load certificate
    LoadCertificates(ctx, crt_path.data(), key_path.data());
    // FIXME: don't read util connected
    // weird bug
    /* Load the RSA CA certificate into the SSL_CTX structure in order to add updated certificates into CA list */
    if (!SSL_CTX_load_verify_locations(ctx, ca_path.data(), NULL)) {
        cout << "failed to load certificates\n";
        return -1;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    printf("Server listening on port %d\n", server_port);

    // Listen to a port
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    sockaddr.sin_port = htons(server_port);

    if (::bind(socket_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(socket_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // server database
    db = new Database(erase, reset);

    if (!verbose) printf("Silent mode is on.\n");

    while (true) {
        // Grab a connection from the queue
        auto addrlen = sizeof(sockaddr);
        // if (current_user >= LIMIT)
        //     continue;
        // FIXME: don't read util connected
        if (!SSL_CTX_load_verify_locations(ctx, ca_path.data(), NULL)) {
            cout << "failed to load certificates\n";
            return -1;
        }
        int connection = accept(socket_fd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
        tmp_current_user++;
        if (tmp_current_user > LIMIT) {
            printf("%d users attempting to connect. %d users might have to wait.\n", tmp_current_user, tmp_current_user - current_user);

            // TODO: there should be a wait list
            // TODO: maybe create another table called wait
        }
        if (connection < 0) {
            perror("Failed to grab connection");
            exit(EXIT_FAILURE);
        }

        string request = "init";

        // Add some work to the queue
        // tp->queue_work(connection, request);

        // const pair<int, string> query = pair<int, string>(connection, request);
        Connection conn { connection, request };
        thread_pool.push(process_request, conn);
    }

    close(socket_fd);
    SSL_CTX_free(ctx);

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
    delete db;
    for (auto ssl : client_SSLs) {
        SSL_shutdown(ssl.second);
        SSL_free(ssl.second);
    }
    // for (auto& fd : client_fds) {
    //     close(*fd.second);
    //     shutdown(*fd.second, SHUT_RDWR);
    // }

    close(socket_fd);
    shutdown(socket_fd, SHUT_RDWR);
    SSL_CTX_free(ctx);
    termination_flag = true; // danger
    // NOTE: threads will be handled by ctpl
    exit(1);
}

void sigpipe_handler(int unused)
{
    printf("Caught signal %d\n", unused);
    printf("Something is written to a pipe where nothing is read from anymore. A user may just be gone.\n");
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
    // printf("raw cmd: %s\n", cmd.c_str());
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

    if (args.size() == 1 && (args[0] == "Exit" || args[0].find("Exit") != string::npos)) {
        cmd_type = EXIT;
    } else if (args.size() == 1 && (args[0] == "List" || args[0].find("List") != string::npos)) {
        cmd_type = LIST; // Please login first\n
    } else if (args.size() == 2 && args[0] == "REGISTER") {
        cmd_type = REGISTER;
    } else if (args.size() == 2 && is_number(args[1])) {
        cmd_type = LOGIN;
    } else if (args[0] == "TRANSACTION") {
        cmd_type = TRANSACTION;
        // string param;
        // size_t pos;
        // if ((pos = cmd.find_first_of("#")) == string::npos)
        //     pos = cmd.find_first_of("\r\n");
        // param = cmd.substr(0, pos);
        // args.clear();
        // args.push_back("TRANSACTION");
        // args.push_back(cmd.substr(pos + 1));
    } else if (args.size() == 3 && is_number(args[1], true)) {
        // plaintext
        cmd_type = TRANSACTION;
    } else
        cmd_type = ERR;

    return make_pair(cmd_type, args);
}

void process_request(int id, Connection& conn)
{
    // handle the cases where we are terminating the server
    // exit if not init item
    if (conn.request != "init") {
        return;
    }

    // after the connection is given
    current_user++;
    int connection = conn.connection;

    /* Load the RSA CA certificate into the SSL_CTX structure */
    if (!SSL_CTX_load_verify_locations(ctx, ca_path.data(), NULL)) {
        cout << "failed to load certificates\n";
        return;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, connection);
    SSL_accept(ssl);

    X509* crt = SSL_get_peer_certificate(ssl);
    EVP_PKEY* p_key = X509_get_pubkey(crt);
    RSA* rsa_key = EVP_PKEY_get1_RSA(p_key);
    // const char* res = "CONNECTED\r\n";

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
    printf("Connected clients num: %d\n", current_user);
    string username = "";

    while (true && connection && !termination_flag) {
        // Read from the connection
        char buffer[MAX_LENGTH] = {};
        memset(buffer, 0, MAX_LENGTH);
        // int tmp_byte_read = recv(connection, buffer, sizeof(buffer), 0); // RECV_SIGNAL
        // int tmp_byte_read = SSL_read(ssl, buffer, sizeof(buffer) + 1);
        int tmp_byte_read = SSL_read_D(ssl, rsa_key, buffer, MAX_LENGTH, verbose);
        string raw(buffer);

        if (tmp_byte_read == -1 || raw.empty()) {
            int status = db->user_logout(clientip, clientport);

            int online_num = db->user_num();
            for (int i = 0; i < client_fds.size(); i++) {
                if (client_fds[i].first == username) {
                    client_fds.erase(client_fds.begin() + i);
                    client_SSLs.erase(client_SSLs.begin() + i);
                    break;
                }
            }

            // close the connection with this user
            printf("Online num: %d\n", online_num);
            break;
        }

        sprintf(display_msg, "→ [%s] from %s@%d\n", raw.c_str(), clientip, clientport);

        printf("%s\n", display_msg);

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
                response += to_string(status) + " " + "OK\r\n";
            else
                response += to_string(status) + " " + "FAIL\r\n";

            // send(connection, response.c_str(), response.size(), 0);
            // SSL_write(ssl, response.c_str(), response.size() + 1);
            SSL_write_E(ssl, key_path, response, MAX_LENGTH, verbose);
            // printf("%s\n", response.c_str());
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
                client_SSLs.push_back(make_pair(username, ssl));
                auto user = db->user_info(username);
                response = to_string(user.balance) + "\n";
                response += public_key + "\n";
                response += to_string(db->user_num()) + "\n";
                printf("Online num: %d\n", db->user_num());
                response += db->user_list_info();
            }
            // send(connection, response.c_str(), response.size(), 0);
            // SSL_write(ssl, response.c_str(), response.size() + 1);
            SSL_write_E(ssl, key_path, response, MAX_LENGTH, verbose);
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
            // send(connection, response.c_str(), response.size(), 0);
            // SSL_write(ssl, response.c_str(), response.size() + 1);
            SSL_write_E(ssl, key_path, response, MAX_LENGTH, verbose);
            break;
        }
        case TRANSACTION: {
            // DONE: two-stage implementation - encrypted transaction is gauranteed
            // raw processed_cmd[1]
            // printf("transaction\n");
            char* plaintext = new char[RSA_size(rsa_key) + 1];

            int tmp_byte_read = SSL_read_D(ssl, rsa_key, plaintext, MAX_LENGTH, verbose);

            string tmp_raw(plaintext);
            pair<int, vector<string>>
                item = parse_command(tmp_raw);
            cmd_type = item.first;
            vector<string> processed_cmd = item.second;

            int status
                = db->user_transaction(processed_cmd[0], processed_cmd[2], stod(processed_cmd[1]));
            if (status == TRANSFER_OK)
                response = "transfer OK\n";
            else if (status == TRANSFER_FAIL)
                response = "transfer FAIL\n";
            else if (status == TRANSFER_SENDER_BANKRUPT)
                response = "transfer FAIL due to bankruptcy\n";

            // to the sender
            auto sender = db->user_info(processed_cmd[0]);
            auto receiver = db->user_info(processed_cmd[2]);
            int* sender_fd = nullptr;
            int* receiver_fd = nullptr;
            SSL* sender_SSL;
            SSL* receiver_SSL;
            for (int i = 0; i < client_fds.size(); i++) {
                if (client_fds[i].first == sender.username) {
                    sender_fd = client_fds[i].second;
                    sender_SSL = client_SSLs[i].second;
                }
                if (client_fds[i].first == receiver.username) {
                    receiver_fd = client_fds[i].second;
                    receiver_SSL = client_SSLs[i].second;
                }
                if (sender_fd != nullptr && receiver_fd != nullptr)
                    break;
            }

            // verify sender and receiver
            // if ok then send OK to receiver

            int tmp_byte_write = SSL_write_E(sender_SSL, key_path, response, MAX_LENGTH, verbose);

            // SSL_connect(c_ssl);
            // SSL_write(tmp_ssl, ciphertext, r);
            // send(*sender_fd, response.c_str(), response.size(), 0);
            break;
        }
        case EXIT: {
            int status = db->user_logout(clientip, clientport);
            response += "Bye\n";
            // send(connection, response.c_str(), response.size(), 0);
            // SSL_write(ssl, response.c_str(), response.size());
            SSL_write_E(ssl, key_path, response, MAX_LENGTH, verbose);
            int online_num = db->user_num();
            for (int i = 0; i < client_fds.size(); i++) {
                if (client_fds[i].first == username) {
                    client_fds.erase(client_fds.begin() + i);
                    client_SSLs.erase(client_SSLs.begin() + i);
                    break;
                }
            }

            // close the connection with this user
            printf("Online num: %d\n", online_num);
            // close(connection);
            // shutdown(connection, SHUT_RDWR);
            break;
        }
        default: {
            response = to_string(QUERY_ERROR);
            // send(connection, response.c_str(), response.size(), 0);
            // SSL_write(ssl, response.c_str(), response.size() + 1);
            SSL_write_E(ssl, key_path, response, MAX_LENGTH, verbose);
            printf("error: %s\n", response.c_str());
            break;
        }
        }

        if (cmd_type == EXIT)
            break;
    }
    printf("Drop connection with %s@%d\n", clientip, clientport);
    printf("Connected clients num: %d\n", --current_user);
    --tmp_current_user;
    // close(connection);
    // shutdown(connection, SHUT_RDWR);
    SSL_shutdown(ssl);
    SSL_free(ssl);
    return;
}

tuple<bool, bool> check_db_status()
{
    bool erase = true;
    char db_alive;
    while (true) {
        printf("Keep database alive after server termination? [Y/n] ");
        do {
            scanf("%c", &db_alive);
        } while (db_alive == '\n');

        if (db_alive == 'Y' || db_alive == 'y') {
            erase = false;
            break;
        } else if (db_alive == 'n' || db_alive == 'N') {
            erase = true;
            break;
        }
    }
    bool reset = true;
    char db_reset;
    while (true) {
        printf("Reset database during initialization? [Y/n] ");
        do {
            scanf("%c", &db_reset);
        } while (db_reset == '\n');

        if (db_reset == 'Y' || db_reset == 'y') {
            reset = true;
            break;
        } else if (db_reset == 'n' || db_reset == 'N') {
            reset = false;
            break;
        }
    }
    return make_tuple(erase, reset);
}