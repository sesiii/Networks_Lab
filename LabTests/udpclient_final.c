#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    // Creating socket
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setting up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Sending initial message
    char *msg = "Hello server. Doing good?\n";
    sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Message sent to server\n");

    // Receiving response from server
    int n = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
    buffer[n] = '\0';
    printf("Message received from server: %s\n", buffer);

    
        char file_name[BUFFER_SIZE];
        printf("Enter the file name you want to get from the server: ");
        scanf("%s", file_name);

        // Sending file request
        sendto(sock_fd, file_name, strlen(file_name), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        printf("File name being checked on the server side...\n");

        // Receiving server response
        int m = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
        buffer[m] = '\0';
        printf("Received message from server: %s\n", buffer);
        
        // // If the file exists, terminate the client
        // if (strcmp(buffer, "File exists") == 0) {
        //     printf("File exists on the server. Exiting client...\n");
        //     break;
        // }

    

    close(sock_fd);
    return 0;
}