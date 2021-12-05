#include "client.hpp"
#include <arpa/inet.h>
#include <ctype.h>
#include <exception>
#include <iostream>
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

#define CMD_LENGTH 20
#define FAIL -1
#define MAX_LENGTH 1024

const char DEFAULT_IP_ADDRESS[20] = "10.211.55.4";
const int DEFAULT_PORT = 8888;
int SERVER_PORT;
char SERVER_IP_ADDRESS[20];
int server_fd; // server socket fd
char name[20]; // a name for login client
int login_port = 0; // port for client
long bytes_read = 0, bytes_written = 0;
long acct_balance = 0;
long online_num = 0;
vector<vector<string>> peer_list;
string server_public_key;
bool client_server_open = false; // login = true

class not_found_error : public exception
{
    virtual const char* what() const throw()
    {
        return "User not found.";
    }
} not_found;

struct Account
{
    string name;
    string ip_address;
    string port_number;
    long acct_balance;
};

int main(int argc, char const* argv[])
{
    // initialization
    switch (argc) {
    case 3: {
        strcpy(SERVER_IP_ADDRESS, argv[1]);
        SERVER_PORT = atoi(argv[2]);
        break;
    }
    default:
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

    time_t begin = time(NULL);

    // declaration
    int new_socket, valread;
    struct sockaddr_in address;
    int k = 0;

    // creating server's socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // forcefully attaching socket to the port
    bzero(&address, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS); // Parallels "10.211.55.4"
    address.sin_port = htons(SERVER_PORT); /// 8888

    int err = connect(server_fd, (struct sockaddr*)&address, sizeof(address));

    if (err == FAIL) {
        // fail to connect to server
        perror("Connection error");
        exit(EXIT_FAILURE);
    }

    // print the server socket addr and port
    get_info(&address);

    // wait for user inputs
    int opt;
    do {
        printf("Press enter to continue...");
        getchar();
        printf("\033[H\033[J");
        printf("\n*****Select a method:*****\n"
               "1. REGISTER\n"
               "2. LOGIN\n"
               "3. LIST\n"
               "4. TRANSACTION\n"
               "5. EXIT\n");
        printf("\n>");
        scanf("%d", &opt);
        getchar();

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
                // char login_opt;
                // while (true) {
                //     printf("Do you want to continue without logging out?[Y/n] ");
                //     do {
                //         scanf("%c", &login_opt);
                //     } while (login_opt == '\n');
                //     if (login_opt == 'Y') {
                //         client_server_open = false;
                //         break;
                //     } else if (login_opt == 'n') {
                //         break;
                //     }
                // }
                // if (login_opt == 'n') break;
            }

            strcpy(rcv_msg, login_server(server_fd, &login_port));
            // printf("\n%s\n", rcv_msg);
            // listen to other users (always listening)
            vector<string> res = split(rcv_msg, "\n");
            int status = 200;
            if (res.size() <= 3) {
                // sth may happen
                if (res.size() <= 2)
                    status = stoi(res[0]);
                printf("\n%s\n", rcv_msg);
                break;
            }

            if (!client_server_open && status != 220) {
                // create a client server for peer transaction
                int cserver_fd;
                struct sockaddr_in cserver_address;
                int k = 0;
                // Creating socket file descriptor
                if ((cserver_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                    perror("socket failed");
                    exit(EXIT_FAILURE);
                }
                // Forcefully attaching socket to the port

                cserver_address.sin_family = AF_INET;
                cserver_address.sin_addr.s_addr = INADDR_ANY;
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
                pthread_t tid;
                // Creating thread to keep receiving message in real time
                pthread_create(&tid, NULL, &receive_thread, &cserver_fd);
                client_server_open = true;

                // show information after creation
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
            // printf("\n%s\n", rcv_msg);
            print_sys_info();
            break;
        }
        case TRANSACTION: { // transaction p2p
            // request list of users from the server
            request_list(server_fd);
            try {
                strcpy(rcv_msg, p2p_transaction(server_fd));
            } catch (exception& e) {
                // user not found
                break;
            }
            printf("\n%s\n", rcv_msg);
            printf("Renewing list after transaction...");
            request_list(server_fd);
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
    // printf("From: ");
    // scanf("%s", sender);
    // getchar();
    strcpy(sender, name);
    printf("To: ");
    scanf("%s", receiver);
    getchar();
    printf("Amount of money to transfer: ");
    scanf("%d", &amount);
    getchar();

    char* transact_msg = new char[MAX_LENGTH];
    strcat(transact_msg, sender);
    strcat(transact_msg, "#");
    strcat(transact_msg, to_string(amount).c_str());
    strcat(transact_msg, "#");
    strcat(transact_msg, receiver);
    // receive confirm message from server
    char* rcv_msg = new char[MAX_LENGTH];

    // Connect to peer socket
    int peer_sock = 0;
    struct sockaddr_in peer_serv_addr;
    peer_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (peer_sock < 0) {
        printf("\n Socket creation error \n");
    }

    int host_port;
    try {
        // find user
        vector<string> peer_info = find_peer_info(receiver);
        host_port = stoi(peer_info[2]);
    } catch (exception& e) {
        cout << e.what() << '\n';
        throw not_found;
    }

    // bzero(&peer_serv_addr, sizeof(peer_serv_addr));
    peer_serv_addr.sin_family = AF_INET;
    peer_serv_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY always gives an IP of 0.0.0.0
    peer_serv_addr.sin_port = htons(host_port);

    int err = connect(peer_sock, (struct sockaddr*)&peer_serv_addr, sizeof(peer_serv_addr));
    if (err == FAIL) {
        printf("\nConnection Failed \n");
    }
    char buffer[MAX_LENGTH] = { 0 };
    strcpy(buffer, transact_msg);
    // sending
    bytes_written += send(peer_sock, buffer, sizeof(buffer), 0);
    // printf("msg sent: %s\n", buffer);
    char tmp_msg_from_peer[MAX_LENGTH] = { 0 };
    // bytes_read += recv(peer_sock, tmp_msg_from_peer, MAX_LENGTH, 0); // peer will recv from server
    close(peer_sock);
    // printf("msg from peer: %s\n", tmp_msg_from_peer);
    bytes_read += recv(server_fd, rcv_msg, MAX_LENGTH, 0);

    return rcv_msg; // transfer OK
}

vector<string> find_peer_info(char* name)
{
    if (peer_list.size() == 0) {
        printf("There is currently no online users.\n");
        throw not_found;
    } else {
        // string tmp_name(name);
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
    struct sockaddr_in address;
    char buffer[2000] = { 0 };
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;

    // Initialize my current set
    FD_ZERO(&current_sockets);
    FD_SET(socket_fd, &current_sockets);
    int k = 0;
    while (1) {
        k++;
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_sockets)) {

                if (i == socket_fd) {
                    int client_socket;

                    if ((client_socket = accept(socket_fd, (struct sockaddr*)&address,
                             (socklen_t*)&addrlen))
                        < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_socket, &current_sockets);
                } else {
                    // receiving
                    int tmp_byte_read = recv(i, buffer, sizeof(buffer), 0);
                    // printf("\n%s\n", buffer);
                    send(server_fd, buffer, tmp_byte_read, 0);
                    bytes_read += tmp_byte_read;
                    // char tmp_rcv_from_server[2000] = { 0 };
                    // int valread_from_server = recv(server_fd, tmp_rcv_from_server, sizeof(tmp_rcv_from_server), 0);
                    // printf("peer from server: %s\n", tmp_rcv_from_server);
                    // send(i, tmp_rcv_from_server, valread_from_server, 0);
                    FD_CLR(i, &current_sockets);
                }
            }
        }

        if (k == (FD_SETSIZE * 2))
            break;
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
        "\n*****List Info*****\n"
        "Username: %s\n"
        "Account Balance: %ld\n"
        "Server Public Key: %s\n"
        "Online User #: %ld\n",
        name, acct_balance, server_public_key.c_str(), online_num);

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

    printf("Enter username: ");
    scanf("%s", name);
    getchar();
    strcat(snd_msg, name);
    int snd_byte = send(socket_fd, snd_msg, sizeof(snd_msg) + 1, 0);
    bytes_written += snd_byte;

    int rcv_byte = recv(socket_fd, rcv_msg, MAX_LENGTH, 0);
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
        printf("Are you a current user?[Y/n] ");
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
    printf("Enter port: ");
    scanf("%s", user_port);
    getchar();
    string tmp_s(user_port);

    *login_port = stoi(tmp_s);

    strcat(snd_msg, user_port);
    int snd_byte = send(socket_fd, snd_msg, sizeof(snd_msg) + 1, 0);
    bytes_written += snd_byte;

    int rcv_byte = recv(socket_fd, rcv_msg, MAX_LENGTH, 0);
    bytes_read += rcv_byte;
    return rcv_msg;
}

char* request_list(int socket_fd)
{
    const char* snd_msg = "List";
    char* rcv_msg = new char[MAX_LENGTH];
    int snd_byte = send(socket_fd, snd_msg, sizeof(snd_msg) + 1, 0);
    bytes_written += snd_byte;

    int rcv_byte = recv(socket_fd, rcv_msg, MAX_LENGTH, 0);
    bytes_read += rcv_byte;
    // parse_info
    // FIXME: way to identify error
    if (strcmp(rcv_msg, "Please login first\n") != 0) {
        parse_list_info(rcv_msg);
    }

    return rcv_msg;
}

char* exit_server(int socket_fd)
{
    const char* exit_msg = "Exit";
    char* rcv_msg = new char[MAX_LENGTH];
    send(socket_fd, exit_msg, sizeof(exit_msg) + 1, 0);
    recv(socket_fd, rcv_msg, MAX_LENGTH, 0);
    close(socket_fd);
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
