//Assignment 2 Submission
//Name: Dadi Sasank Kumar
//Roll number: 22CS10020
//Link of the pcap file: https://drive.google.com/file/d/1xHfJ-ncZlSxzedsbVnj_decCiA74deIN/view?usp=sharing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 1020
#define MAXLINE 1000

int main() {
    int serverfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    
    // Create a UDP Socket
    serverfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Clear servaddr
    bzero(&servaddr, sizeof(servaddr));
    bzero(&cliaddr, sizeof(cliaddr));

    // Set server address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind server address to socket descriptor
    if (bind(serverfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(serverfd);
        exit(EXIT_FAILURE);
    }

    printf("Server running...\n");

    len = sizeof(cliaddr);

    // Receive file name from client
    int n = recvfrom(serverfd, buffer, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
    buffer[n] = '\0';
    printf("Received file request for: %s\n", buffer);

    FILE *file = fopen(buffer, "r");
    if (file == NULL) {
        // Send NOTFOUND message to client
        snprintf(buffer, sizeof(buffer), "NOTFOUND %s", buffer);
        sendto(serverfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cliaddr, len);
        printf("File not found. NOTFOUND message sent to client.\n");
        close(serverfd);
        return 0;
    }

    // Reads the first line (HELLO)
    fgets(buffer, sizeof(buffer), file);
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

    sendto(serverfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cliaddr, len);
    printf("Sent HELLO message to client.\n");

    // Send words to client
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character
        sendto(serverfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cliaddr, len);
        printf("Sent word: %s\n", buffer);
        if (strcmp(buffer, "FINISH") == 0) {
            break;
        }
    }

    fclose(file);
    close(serverfd);
    printf("Server terminated.\n");

    return 0;
}