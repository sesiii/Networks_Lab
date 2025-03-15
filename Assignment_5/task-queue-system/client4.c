//Name: Dadi Sasank Kumar
//R NO: 22CS10020
//Assignment 5
// client:  program to demonstrate the client that connects to the server and does nothing


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER 1024

int main() {
    const char *server_ip = "127.0.0.1";

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connecting to server, staying idle...\n");
    char buffer[MAX_BUFFER];
    printf("Client %d: Connected to server at %s:%d\n", getpid(), server_ip, PORT);

    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        int bytes_read = read(client_socket, buffer, MAX_BUFFER - 1);
        
        if (bytes_read <= 0) {
            printf("Client %d: Server closed the connection. Exiting...\n", getpid());
            break;
        }

        buffer[bytes_read] = '\0';  
        printf("Server: %s\n", buffer);

        if (strcmp(buffer, "exit") == 0) {
            printf("Client %d: Received exit message. Disconnecting...\n", getpid());
            break;
        }

        if (strncmp(buffer, "No tasks available", 18) == 0) {
            printf("Client %d: No more tasks available. Exiting...\n", getpid());
            break;
        }

        sleep(1);
    }

    close(client_socket);
    return 0;
}