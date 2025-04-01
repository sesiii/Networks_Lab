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
    int client_sock = *(int*)arg;
    char buffer[BUFFER_SIZE];
    int n = read(client_sock, buffer, BUFFER_SIZE);
    buffer[n] = '\0';
    printf("Mail received: %s\n", buffer);
    write(client_sock, "Mail received", 13);
    close(client_sock);
    free(arg);
    return NULL;
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(sockfd, MAX_CLIENTS);
    printf("Mail Server on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);

        int *arg = malloc(sizeof(int));
        *arg = client_sock;
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, arg);
        pthread_detach(thread);
    }
    close(sockfd);
    return 0;
}