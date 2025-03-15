//Name: Dadi Sasank Kumar
//R NO: 22CS10020
//Assignment 5
// client: program to demonstrate a client that requests a task but does not respond


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024
#define PORT 8080

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
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    
    char buffer[MAX_BUFFER];
    strcpy(buffer, "GET_TASK");
    write(client_socket, buffer, strlen(buffer));
    
    printf("Requesting a task but not responding...\n");
    
    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        int bytes_read = read(client_socket, buffer, MAX_BUFFER - 1);
        
        if (bytes_read <= 0) {
            printf("Client %d: Server closed connection. Exiting...\n", getpid());
            break;
        }
        
        buffer[bytes_read] = '\0';

        printf("Client %d: Received: %s\n", getpid(), buffer);
        
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