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
    int window_size = 5;

    while (1) {
        int packets_received = 0;
        while (packets_received < window_size) {
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
            buffer[n] = '\0';
            printf("Packet %d: %s\n", packets_received, buffer);
            packets_received++;
        }
        sendto(sockfd, "ACK", 3, 0, (struct sockaddr*)&client_addr, addr_len);
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
    printf("UDP Flow Server on port %d...\n", PORT);

    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, &sockfd);
    pthread_detach(thread);

    while (1) sleep(1);
    close(sockfd);
    return 0;
}