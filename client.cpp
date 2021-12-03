#include "client.hpp"
#include <arpa/inet.h>
#include <ctype.h>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using namespace std;

#define CMD_LENGTH 20
#define FAIL -1
#define MAX_LENGTH 1024

const char DEFAULT_IP_ADDRESS[20] = "10.211.55.4";
char name[20]; // a name for each client
int PORT;
char IP_ADDRESS[20];
long bytes_read = 0, bytes_written = 0;
long acct_balance = 0;
long online_num = 0;
vector<vector<string>> peer_list;
string server_public_key;

struct Account
{
    string name;
    string ip_address;
    string port_number;
    long acct_balance;
};

int main(int argc, char const* argv[])
{
    switch (argc) {
    case 1: {
        printf("Please specify an IP address and a port number to connect to.\n");
        printf("%s\n", man);
        printf("Now running on default...\n");
        // default
        PORT = 8888;
        strcpy(IP_ADDRESS, DEFAULT_IP_ADDRESS);
        break;
    }
    case 2: {
        printf("Please specify an IP address and a port number to connect to.\n");
        printf("%s\n", man);
        printf("Now running on default...\n");
        // default
        PORT = 8888;
        strcpy(IP_ADDRESS, DEFAULT_IP_ADDRESS);
        break;
    }
    case 3: {
        strcpy(IP_ADDRESS, argv[1]);
        PORT = atoi(argv[2]);
        break;
    }
    default:
        printf("Please specify an IP address and a port number to connect to.\n");
        printf("%s\n", man);
        printf("Now running on default...\n");
        // default
        PORT = 8888;
        strcpy(IP_ADDRESS, DEFAULT_IP_ADDRESS);
        break;
    }

    // Socket programming declaration
    int socket_fd, new_socket, valread;
    struct sockaddr_in address;
    int k = 0;

    // Creating client's socket file descriptor
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    bzero(&address, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(IP_ADDRESS); // Parallels "10.211.55.4"
    address.sin_port = htons(PORT); /// 8888

    int err = connect(socket_fd, (struct sockaddr*)&address, sizeof(address));

    if (err == FAIL) {
        // fail to connect to server
        perror("Connection error");
        exit(EXIT_FAILURE);
    }

    // Print the server socket addr and port
    get_info(&address);

    // TODO: listen to other users
    // int server_fd;
    // peer_setup(server_fd);

    // TODO: wait for user inputs
    int opt;
    do {
        printf("\n*****Select a method:*****\n"
               "1. REGISTER\n"
               "2. LOGIN\n"
               "3. LIST\n"
               "4. TRANSACTION\n"
               "5. EXIT\n");
        printf("\n>");
        scanf("%d", &opt);
        // char* rcv_msg = (char*)malloc(sizeof(char) * MAX_LENGTH);
        char* rcv_msg = new char[MAX_LENGTH];

        switch (opt) {
        case REGISTER: {
            // register
            strcpy(rcv_msg, register_user(socket_fd));
            printf("\n%s\n", rcv_msg);
            break;
        }
        case LOGIN: {
            strcpy(rcv_msg, login_server(socket_fd));
            printf("\n%s\n", rcv_msg);
            break;
        }
        case LIST: { // list
            strcpy(rcv_msg, request_list(socket_fd));
            printf("\n%s\n", rcv_msg);
            break;
        }
        case TRANSACTION: { // transaction p2p
            // TODO: listen to other users
            break;
        }
        case EXIT: {
            // exit
            strcpy(rcv_msg, exit_server(socket_fd));
            printf("\n*****Session ended*****\nConnection closed\n");
            break;
        }
        default:
            printf("\nCommand not found\n");
        }
    } while (opt != EXIT);

    return 0;
}

// peer server setup
void peer_setup(int server_fd)
{
    // https://stackoverflow.com/a/65102270/10871988
    // calling the function from global namespace
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Failed to establish socket for P2P transaction");
        exit(EXIT_FAILURE);
    }
    if (::bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    pthread_t tid;

    printf("Successfully connected to client server");
    get_info(&address);

    // Creating thread to keep receiving message in real time
    // pthread_create(&tid, NULL, &receive_thread, &server_fd);
}

// Sending messages to port
void sending()
{

    char buffer[2000] = { 0 };
    // Fetching port number
    int PORT_server;

    // IN PEER WE TRUST
    printf("Enter the port to send message: "); // Considering each peer will enter different port
    scanf("%d", &PORT_server);

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char hello[1024] = { 0 };
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS); // INADDR_ANY always gives an IP of 0.0.0.0
    serv_addr.sin_port = htons(PORT_server);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return;
    }

    char dummy;
    printf("Enter your message: ");
    scanf("%c", &dummy); // The buffer is our enemy
    scanf("%[^\n]s", hello);
    sprintf(buffer, "%s[PORT:%d] says: %s", name, PORT, hello);
    send(sock, buffer, sizeof(buffer), 0);
    printf("\nMessage sent\n");
    close(sock);
}

// Calling receiving infinitely
void* receive_thread(void* socket_fd)
{
    int s_fd = *((int*)socket_fd);
    while (1) {
        receiving(s_fd);
    }
}

// Receiving messages on our port
void receiving(int socket_fd)
{
    struct sockaddr_in address;
    int valread;
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
                    valread = recv(i, buffer, sizeof(buffer), 0);
                    printf("\n%s\n", buffer);
                    FD_CLR(i, &current_sockets);
                }
            }
        }

        if (k == (FD_SETSIZE * 2))
            break;
    }
}

void get_info(struct sockaddr_in* address)
{
    printf("IP address is: %s\n", inet_ntoa(address->sin_addr));
    printf("port is: %d\n", (int)ntohs(address->sin_port));
    return;
}

int check_command(char* msg)
{
    char* search_result = strchr(msg, '#');
    // char* cmd_type = (char*)malloc(sizeof(char) * CMD_LENGTH);
    int cmd_type = 0;

    if (search_result == NULL) {
        // no # was found
        if (strcmp(msg, "Exit") == 0)
            cmd_type = EXIT;
        else if (strcmp(msg, "List") == 0)
            cmd_type = LIST;
        else
            cmd_type = ERR;
    } else {
        char* user_msg = strchr(search_result + 1, '#');
        if (user_msg == NULL) // only one # was found
            if (isdigit(search_result[1]) != 0)
                cmd_type = LOGIN;
            else
                cmd_type = REGISTER;
        else {
            // more than one # was found
            if (isdigit(search_result[1]) != 0)
                cmd_type = TRANSACTION;
            else
                cmd_type = ERR;
        }
    }

    return cmd_type;
}

void parse_info(char* msg)
{
    vector<string> tmp = split(string(msg), "\n");
    acct_balance = stoi(tmp[0]);
    server_public_key = tmp[1];
    online_num = stoi(tmp[2]);
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
    strcat(snd_msg, name);
    int snd_byte = send(socket_fd, snd_msg, sizeof(snd_msg) + 1, 0);
    bytes_written += snd_byte;

    int rcv_byte = recv(socket_fd, rcv_msg, MAX_LENGTH, 0);
    bytes_read += rcv_byte;

    return rcv_msg;
}

char* login_server(int socket_fd)
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
            break;
        }
    }

    strcat(snd_msg, "#");

    char user_port[6];
    printf("Enter port: ");
    scanf("%s", user_port);
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
    return rcv_msg;
}

char* exit_server(int socket_fd)
{
    const char* exit_msg = "Exit";
    char* rcv_msg = new char[MAX_LENGTH];
    send(socket_fd, exit_msg, sizeof(exit_msg), 0);
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
