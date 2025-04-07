#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int su(int a, int b) {
    return a + b;
}

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(sock_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d\n", PORT);

    fd_set readfds;
    int client_socks[MAX_CLIENTS] = {0}; // Array to store client sockets
    int max_fd = sock_fd;

    while (1) {
        FD_ZERO(&readfds);           // Clear the set
        FD_SET(sock_fd, &readfds);   // Add server socket to set

        // Add all client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socks[i] > 0) {
                FD_SET(client_socks[i], &readfds);
                if (client_socks[i] > max_fd) max_fd = client_socks[i];
            }
        }

        // Wait for activity on any socket
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        // New connection
        if (FD_ISSET(sock_fd, &readfds)) {
            int client_sock = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_sock < 0) {
                perror("Accept failed");
                continue;
            }
            char client_ip[16];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 16);
            int client_port = ntohs(client_addr.sin_port);
            printf("Client connected from %s:%d\n", client_ip, client_port);

            // Add new client socket to array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socks[i] == 0) {
                    client_socks[i] = client_sock;
                    if (client_sock > max_fd) max_fd = client_sock;
                    break;
                }
            }
        }

        // Check each client for data
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_sock = client_socks[i];
            if (client_sock > 0 && FD_ISSET(client_sock, &readfds)) {
                int a_, b_;
                char client_ip[16];
                int client_port;

                // Read first integer
                if (read(client_sock, &a_, sizeof(int)) <= 0) {
                    // Client disconnected
                    close(client_sock);
                    client_socks[i] = 0;
                    printf("Client disconnected\n");
                    continue;
                }
                write(client_sock, "ACK", 3);
                printf("Received from client: %d\n", a_);

                // Read second integer
                if (read(client_sock, &b_, sizeof(int)) <= 0) {
                    close(client_sock);
                    client_socks[i] = 0;
                    printf("Client disconnected\n");
                    continue;
                }
                write(client_sock, "ACK", 3);
                printf("Received from client: %d\n", b_);

                // Calculate and send sum
                int c = su(a_, b_);
                write(client_sock, &c, sizeof(int));
                printf("Sent sum of %d and %d: %d\n", a_, b_, c);
            }
        }
    }

    close(sock_fd);
    return 0;
}