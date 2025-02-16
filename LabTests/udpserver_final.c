#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_in client_addr, server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    // Creating socket
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setting up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Binding the socket
    if (bind(sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP Server listening on port %d\n", PORT);

    // Handling initial client message
    int n = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
    buffer[n] = '\0';
    printf("Received from client: %s\n", buffer);

    const char *msg = "Hi client, I just received your message! \nTake care..";
    sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr *)&client_addr, addr_len);
    printf("Message sent to client.\n");

    while (1) {  // Continuous server loop
        // Receiving file name request from client
        int m = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        buffer[m] = '\0';

        printf("Received file name request from client: %s\n", buffer);

        FILE *file = fopen(buffer, "r");
        if (file) {
            fclose(file);
            char *msg = "File exists";
            sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr *)&client_addr, addr_len);
            // close(sock_fd);
            break;
            
        } else {
            char *msg = "I don't have the file..\n";
            sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr *)&client_addr, addr_len);
            perror("File not found");  // Logging for debugging
            // close(sock_fd);
            break;
        }

    }

    close(sock_fd);
    return 0;
}