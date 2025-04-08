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

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int n = read(client_sock, buffer, BUFFER_SIZE);
    buffer[n] = '\0';
    printf("Received: %s\n", buffer);
    write(client_sock, "ACK", 3);
    close(client_sock);
    exit(0); // Child process exits
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(sockfd, MAX_CLIENTS);
    printf("TCP Server on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);

        // Print client IP and port
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);
        printf("Connection from %s:%d\n", client_ip, client_port);

        pid_t pid = fork(); // Use fork for each client
        if (pid == 0) {
            close(sockfd); // Child doesn’t need listener
            handle_client(client_sock);
        } else {
            close(client_sock); // Parent doesn’t need client socket
        }
    }
    close(sockfd);
    return 0;
}