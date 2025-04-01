#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Get client socket details
    getsockname(sockfd, (struct sockaddr*)&client_addr, &client_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    // Send client IP and port to the server
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Client IP: %s, Client Port: %d", client_ip, client_port);
    write(sockfd, buffer, strlen(buffer));
    printf("Sent: %s\n", buffer);

    // Send additional message
    snprintf(buffer, BUFFER_SIZE, "Hello TCP Server");
    write(sockfd, buffer, strlen(buffer));
    printf("Sent: %s\n", buffer);

    // Receive response from server
    int n = read(sockfd, buffer, BUFFER_SIZE);
    buffer[n] = '\0';
    printf("Received: %s\n", buffer);

    close(sockfd);
    return 0;
}