#include "client.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

char name[20]; // a name for each client
int PORT;
char IPaddress[20];

int main(int argc, char const* argv[])
{
    switch (argc) {
    case 1: {
        printf("Please specify an IP address and a port number to connect to.\n");
        printf("%s\n", man);
        printf("Now running on default...\n");
        // default
        PORT = 8888;
        char tmp_address[] = "127.0.0.1";
        strcpy(IPaddress, tmp_address);
        break;
    }
    case 2: {
        printf("Please specify an IP address and a port number to connect to.\n");
        printf("%s\n", man);
        printf("Now running on default...\n");
        // default
        PORT = 8888;
        char tmp_address[] = "127.0.0.1";
        strcpy(IPaddress, tmp_address);
        break;
    }
    case 3: {
        strcpy(IPaddress, argv[1]);
        PORT = atoi(argv[2]);
        break;
    }
    default:
        break;
    }

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int k = 0;

    // Creating socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(IPaddress); // Parallels "10.211.55.4"
    address.sin_port = htons(PORT); /// 8888

    // Printed the server socket addr and port
    get_info(&address);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    int ch;
    pthread_t tid;

    // Creating thread to keep receiving message in real time
    pthread_create(&tid, NULL, &receive_thread, &server_fd);
    do {
        printf("\n*****Select a method from the following:*****\n1. Send message\n0. Quit\n");
        printf("\nEnter choice:");
        scanf("%d", &ch);
        switch (ch) {
        case 1:
            sending();
            break;
        case 0:
            printf("\nLeaving\n");
            break;
        default:
            printf("\nWrong choice\n");
        }
    } while (ch);

    return 0;
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
    serv_addr.sin_addr.s_addr = inet_addr(IPaddress); // INADDR_ANY always gives an IP of 0.0.0.0
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
void* receive_thread(void* server_fd)
{
    int s_fd = *((int*)server_fd);
    while (1) {
        receiving(s_fd);
    }
}

// Receiving messages on our port
void receiving(int server_fd)
{
    struct sockaddr_in address;
    int valread;
    char buffer[2000] = { 0 };
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;

    // Initialize my current set
    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);
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

                if (i == server_fd) {
                    int client_socket;

                    if ((client_socket = accept(server_fd, (struct sockaddr*)&address,
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
}