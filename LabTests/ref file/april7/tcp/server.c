#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int n = read(client_sock, buffer, BUFFER_SIZE - 1);
    if (n <= 0) {
        perror("Read error or client disconnected");
        close(client_sock);
        exit(1);
    }
    buffer[n] = '\0';
    printf("Received: %s\n", buffer);
    write(client_sock, "ACK", 3);
    close(client_sock);
    exit(0);
}

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    signal(SIGCHLD, SIG_IGN); // Prevent zombie processes

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock_fd);
        exit(1);
    }

    if (listen(sock_fd, 5) < 0) {
        perror("Listen failed");
        close(sock_fd);
        exit(1);
    }

    printf("TCP server listening on port %d...\n", PORT);

    while (1) {
        int client_sock = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        char client_ip[1000];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 1000);
        int client_port = ntohs(client_addr.sin_port);
        printf("Client connected from: %s:%d\n", client_ip, client_port);

        pid_t pid = fork();
        if (pid == 0) { // Child process
            close(sock_fd);
            handle_client(client_sock);
        } else if (pid > 0) { // Parent process
            close(client_sock);
        } else {
            perror("Fork failed");
        }
    }

    close(sock_fd);
    return 0;
}