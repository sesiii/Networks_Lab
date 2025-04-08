#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Non-blocking server with select
int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;

    // Set socket to non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(sockfd, MAX_CLIENTS);
    printf("Non-blocking Server on port %d...\n", PORT);

    fd_set readfds;
    int client_socks[MAX_CLIENTS] = {0};
    int max_fd = sockfd;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socks[i] > 0) FD_SET(client_socks[i], &readfds);
            if (client_socks[i] > max_fd) max_fd = client_socks[i];
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &readfds)) {
            int client_sock = accept(sockfd, NULL, NULL);
            if (client_sock >= 0) {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_socks[i] == 0) {
                        client_socks[i] = client_sock;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socks[i] && FD_ISSET(client_socks[i], &readfds)) {
                char buffer[BUFFER_SIZE];
                int n = read(client_socks[i], buffer, BUFFER_SIZE);
                if (n > 0) {
                    buffer[n] = '\0';
                    printf("Received: %s\n", buffer);
                    write(client_socks[i], "ACK", 3);
                } else {
                    close(client_socks[i]);
                    client_socks[i] = 0;
                }
            }
        }
    }
    close(sockfd);
    return 0;
}