#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Step 1: Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Step 2: Bind the socket to a port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Step 3: Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("TCP Server listening on port %d...\n", PORT);

    // Step 4: Accept a client connection
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Client accept failed");
        exit(EXIT_FAILURE);
    }
    printf("Client connected.\n");

    // Step 5: Receive a message from the client
    int n = read(client_fd, buffer, BUFFER_SIZE);
    buffer[n] = '\0';
    printf("Received from client: %s\n", buffer);

    // Send acknowledgment to client
    const char *ack_msg = "Hello, client! Connection established.";
    write(client_fd, ack_msg, strlen(ack_msg));

    while (1) {
        // Step 6: Receive file name request from client
        int bytes_received = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) {
            printf("Client disconnected. Closing connection...\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("Client requested file: %s\n", buffer);

        // Step 7: Check if file exists
        FILE *file = fopen(buffer, "r");
        if (file) {
            fclose(file);
            const char *response = "File exists";
            write(client_fd, response, strlen(response));
            printf("File found. Informing client.\n");
        } else {
            const char *response = "File does not exist";
            write(client_fd, response, strlen(response));
            printf("File not found. Informing client.\n");
        }
    }

    // Step 8: Close the connection
    close(client_fd);
    close(server_fd);
    return 0;
}