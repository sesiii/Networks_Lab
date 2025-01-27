//Assignment 2 Submission
//Name: Dadi Sasank Kumar
//Roll number: 22CS10020
//Link of the pcap file: https://drive.google.com/file/d/1URpFWPYgQA34iylWoNBFYTAL37D3-kbV/view?usp=sharing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 5001
#define MAXLINE 1000

int main() {
    int serverfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    
    // Create UDP Socket
    serverfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    bzero(&servaddr, sizeof(servaddr));
    bzero(&cliaddr, sizeof(cliaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(serverfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(serverfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server running...\n");
    len = sizeof(cliaddr);
    
    // Receive filename
    int n = recvfrom(serverfd, buffer, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
    buffer[n] = '\0';
    printf("Received file request for: %s\n", buffer);
    
    // Open requested file
    FILE *file = fopen(buffer, "r");
    if (file == NULL) {
        snprintf(buffer, sizeof(buffer), "NOTFOUND %s", buffer);
        sendto(serverfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cliaddr, len);
        printf("File not found. NOTFOUND message sent to client.\n");
        close(serverfd);
        return 0;
    }
    
    // Send HELLO (first line)
    fgets(buffer, sizeof(buffer), file);
    buffer[strcspn(buffer, "\n")] = '\0';
    sendto(serverfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cliaddr, len);
    printf("Sent HELLO message to client.\n");
    
    // Process word requests
    while (1) {
        // Receive word request
        n = recvfrom(serverfd, buffer, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';
        printf("Received request: %s\n", buffer);
        
        // Read next word from file
        char word[MAXLINE];
        if (fgets(word, sizeof(word), file) != NULL) {
            word[strcspn(word, "\n")] = '\0';
            sendto(serverfd, word, strlen(word), 0, (struct sockaddr *)&cliaddr, len);
            printf("Sent word: %s\n", word);
            
            if (strcmp(word, "FINISH") == 0) {
                break;
            }
        }
    }
    
    fclose(file);
    close(serverfd);
    printf("Server terminated.\n");
    return 0;
}