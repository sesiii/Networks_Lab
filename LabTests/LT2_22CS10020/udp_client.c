

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#define PORT 9090
#define BUFFER_SIZE 1024

int main()
{
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];
    
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;

    while (1) {
        char msg[100];
        printf("Write a message for the server: ");
        fgets(msg, BUFFER_SIZE, stdin);

        sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        int n = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
        buffer[n] = '\0';
        
        printf("Received from server: %s", buffer);
    }

    close(sock_fd);
    return 0;
}
