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

void *handle_client(void *arg) {
    int sockfd = *(int*)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Received from %s:%d: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
            sendto(sockfd, "ACK", 3, 0, (struct sockaddr*)&client_addr, addr_len);
        }
    }
    return NULL;
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("UDP Server on port %d...\n", PORT);

    // Use threads for handling multiple clients
    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, &sockfd);
    pthread_detach(thread);

    while (1) sleep(1); // Keep server running
    close(sockfd);
    return 0;
}