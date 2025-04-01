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
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    char buffer[BUFFER_SIZE] = "Subject: Test\nHello Mail Server";
    write(sockfd, buffer, strlen(buffer));
    printf("Sent: %s\n", buffer);

    int n = read(sockfd, buffer, BUFFER_SIZE);
    buffer[n] = '\0';
    printf("Received: %s\n", buffer);

    close(sockfd);
    return 0;
}