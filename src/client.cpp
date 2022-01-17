#include "client.hpp"
#include "util.hpp"

using namespace std;

#define CMD_LENGTH 20
#define FAIL -1
#define MAX_LENGTH 2048
#define CRLF "\r\n"

// global variables
const char DEFAULT_IP_ADDRESS[20] = "127.0.0.1"; // Parallels "10.211.55.4"
const int DEFAULT_PORT = 8888; // 8888
int SERVER_PORT;
char SERVER_IP_ADDRESS[20];
int server_fd; // server socket fd
char name[MAX_LENGTH]; // a name for login client
int login_port = 0; // port for client
long bytes_read = 0, bytes_written = 0;
long acct_balance = 0;
long online_num = 0;
vector<vector<string>> peer_list;
string server_public_key;
bool client_server_open = false; // login = true
int cserver_fd; // client server

string cert_path = "certs";
// string cert_dir = "certs/";
string target = "client";
string pem_name = "client";
string uid;

// TODO: no initialization
// string key_path = cert_path + "/" + target + ".key";
string key_path = ""; // cert_path + "/" + target + ".key";
// string crt_path = cert_path + "/" + target + ".crt";
string crt_path = ""; // cert_path + "/" + target + ".crt";
string ca_path = cert_path + "/CA.pem";

SSL_CTX* ctx;
SSL* ssl;

SSL_CTX* client_ctx;
SSL* client_ssl;
// exceptions
class not_found_error : public exception
{
    virtual const char* what() const throw()
    {
        return "User not found.";
    }
} not_found;

class self_trans_error : public exception
{
    virtual const char* what() const throw()
    {
        return "Cannot transfer to youself.";
    }
} self_trans;

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

    // initialization
    bool uid_initialized = false;
    switch (argc) {
    case 3: {
        strcpy(SERVER_IP_ADDRESS, argv[1]);
        SERVER_PORT = atoi(argv[2]);
        break;
    }
    case 4: {
        // FIXME: tmp option
        strcpy(SERVER_IP_ADDRESS, argv[1]);
        SERVER_PORT = atoi(argv[2]);
        uid = (string)argv[3];
        uid_initialized = true;
        break;
    }
    default: {
        // using default
        printf(
            "%s\n"
            "%s\n"
            "%s\n",
            notice, man, default_program_msg);
        // default
        SERVER_PORT = DEFAULT_PORT;
        strcpy(SERVER_IP_ADDRESS, DEFAULT_IP_ADDRESS);
        break;
    }
    }

    // session time
    time_t begin = time(NULL);
    // TODO: init
    // FIXME: to dynamic
    if (!uid_initialized)
        uid = "1";

    uid = create_key_and_certificate(cert_path.c_str(), target.c_str(), pem_name.c_str());
    // sleep for one second
    unsigned int microseconds = 10000;
    usleep(microseconds);

    // TODO: init
    // printf("Using uid %s\n", uid.c_str());
    key_path = cert_path + "/" + uid + "_" + target + ".key";
    crt_path = cert_path + "/" + uid + "_" + target + ".crt";

    ctx = SSL_CTX_new(SSLv23_method());
    LoadCertificates(ctx, crt_path.data(), key_path.data());

    if (!SSL_CTX_load_verify_locations(ctx, ca_path.data(), NULL)) {
        cout << "failed to load certificates\n";
        return -1;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    // declaration for server socket
    struct sockaddr_in address;
    int k = 0;

    // creating server's socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Failed to create a socket. Now aborting.");
        exit(EXIT_FAILURE);
    }

    // forcefully attaching socket to the port
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);
    address.sin_port = htons(SERVER_PORT);

    int err = connect(server_fd, (struct sockaddr*)&address, sizeof(address));

    if (err == FAIL) {
        // fail to connect to server
        perror("Connection error");
        exit(EXIT_FAILURE);
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, server_fd);
    SSL_connect(ssl);

    ShowCerts(ssl);

    // print the server socket addr and port
    get_info(&address);

    // wait for user inputs
    int opt;
    do {
        printf("Press enter to continue...");
        getchar();
        printf("\033[H\033[J");
        printf("\n*****Please select a method to continue*****\n\n"
               "1) [REGISTER] Register a user\n"
               "2) [LOGIN] Log in to server\n"
               "3) [LIST] Get latest updates\n"
               "4) [TRANSACTION] Make a transaction\n"
               "5) [EXIT] Log out from server\n");
        // option
        if (client_server_open) {
            printf("\n[%s] ", name);
        } else
            printf("\n ");
        scanf("%d", &opt);
        getchar();
        // end option

        char* rcv_msg = new char[MAX_LENGTH];

        switch (opt) {
        case REGISTER: {
            // register
            strcpy(rcv_msg, register_user(server_fd));
            printf("\n%s\n", rcv_msg);
            break;
        }
        case LOGIN: {
            if (client_server_open) {
                printf("You have previously logged in with %s on port %d.\nPlease consider open another session or exit to continue with another user.\n", name, login_port);
                break;
            }

            strcpy(rcv_msg, login_server(server_fd, &login_port));

            // listen to other users (always listening)
            vector<string> res = split(rcv_msg, "\n");
            int status = 100; // 100 OK
            if (res.size() <= 3) {
                // // sth may happen
                // if (res.size() <= 2)
                //     status = stoi(res[0]);
                printf("\n%s\n", rcv_msg);
                break;
            }

            if (!client_server_open && status != 220) {
                // if not equal to AUTH_FAIL
                // create a client server for peer transaction
                struct sockaddr_in cserver_address;
                int k = 0;

                // Creating socket file descriptor
                if ((cserver_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                    perror("Failed to create a socket. Now aborting.");
                    exit(EXIT_FAILURE);
                }

                // ssl
                client_ctx = SSL_CTX_new(SSLv23_method());
                LoadCertificates(client_ctx, crt_path.data(), key_path.data());

                if (!SSL_CTX_load_verify_locations(client_ctx, ca_path.data(), NULL)) {
                    cout << "failed to load certificates\n";
                    return -1;
                }
                SSL_CTX_set_verify(client_ctx, SSL_VERIFY_PEER, NULL);

                // Forcefully attaching socket to the port
                cserver_address.sin_family = AF_INET;
                cserver_address.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
                cserver_address.sin_port = htons(login_port);
                if (::bind(cserver_fd, (struct sockaddr*)&cserver_address, sizeof(cserver_address)) < 0) {
                    perror("bind failed");
                    break;
                    // exit(EXIT_FAILURE);
                }
                if (listen(cserver_fd, 5) < 0) {
                    perror("listen");
                    break;
                    // exit(EXIT_FAILURE);
                }

                // Thread pool

                // auto num_of_threads
                //     = thread::hardware_concurrency();
                // if (num_of_threads == 0) {
                //     num_of_threads = 1;
                // }

                // ctpl::thread_pool thread_pool(num_of_threads);

                pthread_t tid;
                // Creating thread to keep receiving message in real time
                pthread_create(&tid, NULL, &receive_thread, &cserver_fd);

                client_server_open = true;

                // show information after creation
                string host_ip;
                request_list(server_fd);
                print_sys_info();
            } else {
                perror("Failed to log in.\n");
            }

            break;
        }
        case LIST: { // list
            strcpy(rcv_msg, request_list(server_fd));
            if (!client_server_open) {
                printf("\n%s\n", rcv_msg);
                break;
            }
            print_sys_info();
            break;
        }
        case TRANSACTION: { // transaction p2p
            // request list of users from the server
            request_list(server_fd);
            try {
                strcpy(rcv_msg, p2p_transaction(server_fd));
            } catch (exception& e) {
                // user not found --> will print out the message in p2p
                // or self transaction
                cout << e.what() << endl;
                break;
            }
            printf("\n%s\n", rcv_msg);
            printf("Renewing list after transaction...\n");
            strcpy(rcv_msg, request_list(server_fd));
            print_sys_info();
            break;
        }
        case EXIT: {
            // exit
            strcpy(rcv_msg, exit_server(server_fd));
            time_t end = time(NULL);
            double time_spent = (double)(end - begin);
            printf("\n*****Session ended*****\n"
                   "Bytes written: %ld / Bytes read: %ld\n"
                   "Elapsed time: %.2f sec(s)\n"
                   "Connection closed\n",
                bytes_written, bytes_read, time_spent);
            break;
        }
        default:
            printf("\nCommand not found\n");
        }
    } while (opt != EXIT);

    return 0;
}

char* p2p_transaction(int socket_fd)
{
    // from whom --> can only transfer if logged in
    // how much
    // to whom
    char sender[CMD_LENGTH];
    int amount;
    char receiver[CMD_LENGTH];

    strcpy(sender, name);
    printf("To: ");
    scanf("%s", receiver);
    getchar();
    printf("Amount of money to transfer: ");
    scanf("%d", &amount);
    getchar();

    if (strcmp(sender, receiver) == 0) {
        // cout << self_trans.what() << endl;
        throw self_trans; // throw back to main
    }

    char* transact_msg = new char[MAX_LENGTH];
    strcat(transact_msg, sender);
    strcat(transact_msg, "#");
    strcat(transact_msg, to_string(amount).c_str());
    strcat(transact_msg, "#");
    strcat(transact_msg, receiver);

    // empty char* to receive confirm message from server
    char* rcv_msg = new char[MAX_LENGTH];

    // Connect to peer socket
    int peer_sock = 0;
    struct sockaddr_in peer_serv_addr;
    peer_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (peer_sock == -1) {
        printf("\n Socket creation error \n");
    }

    SSL_CTX* tmp_ctx;
    SSL* tmp_ssl;
    // string key_path = "certs/client.key";
    // string crt_path = "certs/client.crt";
    tmp_ctx = SSL_CTX_new(SSLv23_method());

    LoadCertificates(tmp_ctx, crt_path.data(), key_path.data());
    SSL_CTX_load_verify_locations(tmp_ctx, ca_path.data(), NULL);
    SSL_CTX_set_verify(tmp_ctx, SSL_VERIFY_PEER, NULL);

    int host_port;
    string host_ip;
    try {
        // find user
        vector<string> peer_info = find_peer_info(receiver);
        host_port = stoi(peer_info[2]);
        host_ip = peer_info[1];
    } catch (exception& e) {
        throw not_found; // display at main
    }

    bzero(&peer_serv_addr, sizeof(peer_serv_addr));
    peer_serv_addr.sin_family = AF_INET;
    peer_serv_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY always gives an IP of 0.0.0.0 //  inet_addr(host_ip.c_str());
    peer_serv_addr.sin_port = htons(host_port);

    int err = connect(peer_sock, (struct sockaddr*)&peer_serv_addr, sizeof(peer_serv_addr));
    if (err == FAIL) {
        printf("\nConnection Failed \n");
    }

    tmp_ssl = SSL_new(tmp_ctx);
    SSL_set_fd(tmp_ssl, peer_sock);
    SSL_connect(tmp_ssl);

    char buffer[MAX_LENGTH] = { 0 };
    strcpy(buffer, transact_msg);
    // sending

    FILE* fp = fopen(key_path.c_str(), "r");
    RSA* p_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);

    int len = RSA_size(p_key);
    // printf("len: %d\n", len);
    // printf("buffer text: %s\n", buffer);
    char* ciphertext = new char[len + 1];
    memset(ciphertext, 0, len + 1);
    RSA_private_encrypt((strlen(buffer) + 1) * sizeof(char), (const unsigned char*)buffer, (unsigned char*)ciphertext, p_key, RSA_PKCS1_PADDING);

    SSL_write(tmp_ssl, ciphertext, len);

    char tmp_msg_from_peer[MAX_LENGTH] = { 0 };

    X509* s_crt = SSL_get_peer_certificate(ssl);
    EVP_PKEY* s_p_key = X509_get_pubkey(s_crt);
    RSA* s_rsa_key = EVP_PKEY_get1_RSA(s_p_key);

    bytes_read += SSL_read(ssl, buffer, 256);

    // printf(">>> %s\n", buffer);

    printf("\n[System Info] Encrypted message from Server - <%s>\n", buffer);

    char* plaintext = new char[MAX_LENGTH];
    memset(plaintext, 0, MAX_LENGTH);
    RSA_public_decrypt(RSA_size(s_rsa_key), (unsigned char*)rcv_msg, (unsigned char*)plaintext, s_rsa_key, RSA_PKCS1_PADDING);

    memset(rcv_msg, 0, MAX_LENGTH);
    strcpy(rcv_msg, plaintext);
    // // memset(rcv_msg, 0, MAX_LENGTH);
    // // strcpy(rcv_msg, plaintext);
    SSL_shutdown(tmp_ssl);
    SSL_free(tmp_ssl);
    close(peer_sock);

    return rcv_msg; // transfer OK
}

vector<string> find_peer_info(char* name)
{
    if (peer_list.size() == 0) {
        printf("There is currently no online users.\n");
        throw not_found;
    } else {
        for (int i = 0; i < peer_list.size(); i++) {
            if (strcmp(name, peer_list[i][0].c_str()) == 0) {
                return peer_list[i];
            }
        }
    }
    throw not_found;
}

// Calling receiving infinitely
void* receive_thread(void* socket_fd)
{
    int s_fd = *((int*)socket_fd);
    while (1) {
        int status = receiving(s_fd);
        if (status == -1) {
            break;
        }
    }
    pthread_exit(0);
}

// Receiving messages on our port
int receiving(int socket_fd)
{
    struct sockaddr_in sockaddr;
    char buffer[MAX_LENGTH] = { 0 };
    int addrlen = sizeof(sockaddr);
    auto num_of_threads = thread::hardware_concurrency();
    if (num_of_threads == 0) {
        num_of_threads = 1;
    }

    // int tmp_current_user = 0;
    while (true) {
        // Grab a connection from the queue
        auto addrlen = sizeof(sockaddr);
        // if (current_user >= LIMIT)
        //     continue;
        if (!SSL_CTX_load_verify_locations(client_ctx, ca_path.data(), NULL)) {
            cout << "failed to load certificates\n";
            return -1;
        }
        int connection = accept(socket_fd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);

        if (connection < 0) {
            perror("Failed to grab connection");
            exit(EXIT_FAILURE);
        }

        SSL_CTX_set_verify(client_ctx, SSL_VERIFY_PEER, NULL);
        if (!SSL_CTX_load_verify_locations(client_ctx, ca_path.data(), NULL)) {
            cout << "failed to load certificates\n";
            return -1;
        }

        SSL* tmp_ssl = SSL_new(client_ctx);
        SSL_set_fd(tmp_ssl, connection);
        SSL_accept(tmp_ssl);

        X509* crt = SSL_get_peer_certificate(tmp_ssl);
        EVP_PKEY* p_key = X509_get_pubkey(crt);
        RSA* rsa_key = EVP_PKEY_get1_RSA(p_key);
        int len = RSA_size(rsa_key);

        // int tmp_byte_read = recv(i, buffer, sizeof(buffer), 0);
        int tmp_byte_read = SSL_read(tmp_ssl, buffer, sizeof(buffer));
        // recv done

        bytes_read += tmp_byte_read;

        char* plaintext = new char[len + 1];

        printf("\n[Notification] Someone sent you an encrypted message - <%s>\n", buffer);

        int decrypt_err = RSA_public_decrypt(len, (unsigned char*)buffer, (unsigned char*)plaintext, rsa_key, RSA_PKCS1_PADDING);

        if (decrypt_err == FAIL) {
            printf("decrypt error\n");
            exit(1);
        }
        string tmp_buffer(plaintext);
        vector<string> transfer_info = split(tmp_buffer, "#");
        // plain text done

        // printf("\n>>>\n"); // now
        // printf("\n>>> %s sent you a message\n", transfer_info[0].c_str());

        // start encryption for server
        FILE* fp = fopen(key_path.c_str(), "r");
        RSA* pp_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
        fclose(fp);

        int pp_len = RSA_size(pp_key);
        // printf("len: %d\n", len);
        // printf("buffer text: %s\n", buffer);
        // char* ciphertext = new char[pp_len + 1];
        char* ciphertext = new char[pp_len + 1];
        memset(ciphertext, 0, pp_len + 1);
        int encrypted_length = RSA_private_encrypt((strlen(plaintext) + 1) * sizeof(char), (const unsigned char*)plaintext, (unsigned char*)ciphertext, pp_key, RSA_PKCS1_PADDING);

        char server_prefix[] = "TRANSACTION";

        // send to server

        // SSL_write(ssl, server_prefix, sizeof(server_prefix) + 1);

        SSL_write_E(ssl, key_path, (string)server_prefix, MAX_LENGTH);

        SSL_write(ssl, ciphertext, 256);

        // receiving reponse from server
        // if ok, then show transfer info
        bytes_read += tmp_byte_read;
        // compare buffer, if buffer == "transfer OK" then list transfer info
        // if (strcmp(buffer, "transfer OK\n") == 0) {
        printf("\n[Notification] %s just sent you $%s!\n", transfer_info[0].c_str(), transfer_info[1].c_str());
        // }
        SSL_free(tmp_ssl);
    }
    return -1;
}

void get_info(struct sockaddr_in* address)
{
    printf("Connect to IP address: %s\n", inet_ntoa(address->sin_addr));
    printf("On port: %d\n", (int)ntohs(address->sin_port));
    return;
}

void print_sys_info()
{
    printf(
        "\n*****System Information*****\n"
        "Username: %s\n"
        "Account Balance: %ld\n"
        "Online User #: %ld\n",
        name, acct_balance, online_num);
    // "Server Public Key: %s\n", server_public_key.c_str(),

    if (online_num > 0) {
        printf("Peer List (# - <name>#<IP>#<port>):\n");
        for (int i = 0; i < peer_list.size(); i++) {
            printf("  %d - %s#%s#%s\n", i + 1, peer_list[i][0].c_str(), peer_list[i][1].c_str(), peer_list[i][2].c_str());
        }
    }
    printf("\n");
}

void parse_list_info(char* msg)
{
    vector<string> tmp = split(string(msg), "\n");
    acct_balance = stoi(tmp[0]);

    // X509* s_crt = SSL_get_peer_certificate(ssl);
    // EVP_PKEY* s_p_key = X509_get_pubkey(s_crt);
    // RSA* s_rsa_key = EVP_PKEY_get1_RSA(s_p_key);

    // EVP_PKEY to string
    // server_public_key = LoadPKey(s_p_key);

    server_public_key = tmp[1];

    online_num = stoi(tmp[2]);
    peer_list = vector<vector<string>>();
    for (int i = 3; i < 3 + online_num; i++) {
        peer_list.push_back(split(tmp[i], "#"));
    }
}

char* register_user(int socket_fd)
{
    char snd_msg[MAX_LENGTH] = "REGISTER#";
    char* rcv_msg = new char[MAX_LENGTH];
    char tmp_name[MAX_LENGTH];
    printf("Enter username: ");
    scanf("%s", tmp_name);
    getchar();
    strcat(snd_msg, tmp_name);
    if (!client_server_open) {
        strcpy(name, tmp_name);
    }
    // int snd_byte = send(socket_fd, snd_msg, sizeof(snd_msg) + 1, 0);
    // printf("sending msg: %s\n", snd_msg);
    // int snd_byte = SSL_write(ssl, snd_msg, sizeof(snd_msg) + 1);
    int snd_byte = SSL_write_E(ssl, key_path, (string)snd_msg, MAX_LENGTH);
    // printf("sent msg: %s\n", snd_msg);
    bytes_written += snd_byte;

    // int rcv_byte = recv(socket_fd, rcv_msg, MAX_LENGTH, 0);
    // int rcv_byte = SSL_read(ssl, rcv_msg, MAX_LENGTH);
    X509* crt = SSL_get_peer_certificate(ssl);
    EVP_PKEY* p_key = X509_get_pubkey(crt);
    RSA* rsa_key = EVP_PKEY_get1_RSA(p_key);
    int rcv_byte = SSL_read_D(ssl, rsa_key, rcv_msg, MAX_LENGTH);
    bytes_read += rcv_byte;

    return rcv_msg;
}

char* login_server(int socket_fd, int* login_port)
{
    char snd_msg[MAX_LENGTH] = {};
    char* rcv_msg = new char[MAX_LENGTH];
    char tmp_name[20];
    char auth;
    while (true) {
        printf("Using the JUST registered account?[Y/n] ");
        do {
            scanf("%c", &auth);
        } while (auth == '\n');

        if (auth == 'Y') {
            strcat(snd_msg, name);
            break;
        } else if (auth == 'n') {
            printf("Enter username: ");
            scanf("%s", tmp_name);
            strcat(snd_msg, tmp_name);
            strcpy(name, tmp_name);
            break;
        }
    }

    strcat(snd_msg, "#");

    char user_port[6];
    printf("Enter port (enter 0 if you are not sure which port to use): ");
    scanf("%s", user_port);
    getchar();
    string tmp_s(user_port);

    if (stoi(tmp_s) == 0) {
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
            *login_port = ntohs(tmp_sin.sin_port);
            printf("Requesting %s on port # %d...\n", name, *login_port);
        }
        close(tmp_socket);

    } else
        *login_port = stoi(tmp_s);

    {
        struct sockaddr_in tmp_sin;
        int tmp_socket;

        tmp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tmp_socket == -1) {
            perror("Unknown error occurred. Failed to create a socket. Now aborting.");
            exit(EXIT_FAILURE);
        }

        tmp_sin.sin_port = htons(*login_port);
        tmp_sin.sin_addr.s_addr = INADDR_ANY;
        tmp_sin.sin_family = AF_INET;

        if (::bind(tmp_socket, (struct sockaddr*)&tmp_sin, sizeof(struct sockaddr_in)) == -1) {
            perror("bind failed");
            string tmp_msg = "Couldn't allocate to port # " + to_string(*login_port);
            char* tmp_char = new char[MAX_LENGTH];
            strcpy(tmp_char, tmp_msg.c_str());
            return tmp_char;
        }
        close(tmp_socket);
    }

    strcat(snd_msg, to_string(*login_port).c_str());
    // send message to server
    // int snd_byte = send(socket_fd, snd_msg, sizeof(snd_msg) + 1, 0);
    // bytes_written += snd_byte;

    // int rcv_byte = recv(socket_fd, rcv_msg, MAX_LENGTH, 0);

    int snd_byte = SSL_write_E(ssl, key_path, snd_msg, MAX_LENGTH);
    bytes_written += snd_byte;
    // int rcv_byte = SSL_read(ssl, rcv_msg, MAX_LENGTH);
    X509* crt = SSL_get_peer_certificate(ssl);
    EVP_PKEY* p_key = X509_get_pubkey(crt);
    RSA* rsa_key = EVP_PKEY_get1_RSA(p_key);
    int rcv_byte = SSL_read_D(ssl, rsa_key, rcv_msg, MAX_LENGTH);
    bytes_read += rcv_byte;
    return rcv_msg;
}

char* request_list(int socket_fd)
{
    const char* snd_msg = "List";
    char* rcv_msg = new char[MAX_LENGTH];
    // int snd_byte = send(socket_fd, snd_msg, sizeof(snd_msg) + 1, 0);
    // bytes_written += snd_byte;

    // int rcv_byte = recv(socket_fd, rcv_msg, MAX_LENGTH, 0);

    int snd_byte = SSL_write_E(ssl, key_path, snd_msg, MAX_LENGTH);
    bytes_written += snd_byte;
    // int rcv_byte = SSL_read(ssl, rcv_msg, MAX_LENGTH);
    X509* crt = SSL_get_peer_certificate(ssl);
    EVP_PKEY* p_key = X509_get_pubkey(crt);
    RSA* rsa_key = EVP_PKEY_get1_RSA(p_key);
    int rcv_byte = SSL_read_D(ssl, rsa_key, rcv_msg, MAX_LENGTH);
    bytes_read += rcv_byte;
    // parse_info
    // FIXME: way to identify error --> be aware when self-implemtating the server
    if (strcmp(rcv_msg, "Please login first\n") != 0) {
        parse_list_info(rcv_msg);
    }

    return rcv_msg;
}

char* exit_server(int socket_fd)
{
    if (client_server_open) {
        printf("See you next time, %s!\n", name);
    } else {
        printf("See you next time!\n");
    }

    const char* exit_msg = "Exit";
    char* rcv_msg = new char[MAX_LENGTH];
    // send(socket_fd, exit_msg, sizeof(exit_msg) + 1, 0);
    // const char* req = cmd.c_str();
    // SSL_write(ssl, exit_msg, sizeof(exit_msg) + 1);
    // SSL_read(ssl, rcv_msg, MAX_LENGTH);
    int snd_byte = SSL_write_E(ssl, key_path, exit_msg, MAX_LENGTH);
    bytes_written += snd_byte;
    // int rcv_byte = SSL_read(ssl, rcv_msg, MAX_LENGTH);
    X509* crt = SSL_get_peer_certificate(ssl);
    EVP_PKEY* p_key = X509_get_pubkey(crt);
    RSA* rsa_key = EVP_PKEY_get1_RSA(p_key);

    int rcv_byte = SSL_read_D(ssl, rsa_key, rcv_msg, MAX_LENGTH);

    bytes_read += rcv_byte;
    // recv(socket_fd, rcv_msg, MAX_LENGTH, 0);
    // close(socket_fd);
    // close(socket_fd);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    // TODO: delete keys
    delete_key_and_certificate(uid, cert_path.c_str(), target.c_str(), pem_name.c_str());
    // // cout << rcv_msg << endl;
    // if (string(rcv_msg) == "Bye") {
    // } else {
    //     printf("Failed to close connection with the server.\n");
    // }
    return rcv_msg;
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

void sigint_handler(sig_atomic_t s)
{
    printf("Single Handler: caught signal %d\n", s);
    exit_server(server_fd);
    // if (client_server_open) {
    //     close(cserver_fd);
    // }
    printf("Session terminated.\n");
    exit(1);
}