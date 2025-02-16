#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Step 1: Create a TCP socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Step 2: Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Step 3: Connect to the server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to server.\n");

    // Step 4: Send an initial message
    const char *msg = "Hello, server!";
    write(sock_fd, msg, strlen(msg));

    // Step 5: Receive acknowledgment
    int n = read(sock_fd, buffer, BUFFER_SIZE);
    buffer[n] = '\0';
    printf("Server response: %s\n", buffer);

    while (1) {
        // Step 6: Get filename input from user
        char file_name[BUFFER_SIZE];
        printf("Enter the file name you want to check: ");
        scanf("%s", file_name);

        // Step 7: Send filename request to server
        write(sock_fd, file_name, strlen(file_name));
        printf("File name sent to server.\n");

        // Step 8: Receive server response
        int m = read(sock_fd, buffer, BUFFER_SIZE);
        buffer[m] = '\0';
        printf("Server response: %s\n", buffer);

        // Step 9: Exit if file exists
        if (strcmp(buffer, "File exists") == 0) {
            printf("File found on the server. Exiting client...\n");
            break;
        }
    }

    // Step 10: Close the socket
    close(sock_fd);
    return 0;
}