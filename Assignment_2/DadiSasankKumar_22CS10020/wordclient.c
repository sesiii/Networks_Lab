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
    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr;
    socklen_t len;
    int word_count = 1;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;
    
    // Get file name from user
    char filename[MAXLINE];
    printf("Enter the file name: ");
    scanf("%s", filename);
    
    // Send filename to server
    sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("Sent file request for: %s\n", filename);
    
    len = sizeof(servaddr);
    
    // Receive initial response (HELLO or NOTFOUND)
    int n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *)&servaddr, &len);
    buffer[n] = '\0';
    
    if (strncmp(buffer, "NOTFOUND", 8) == 0) {
        printf("FILE NOT FOUND\n");
        close(sockfd);
        return 0;
    }
    
    // Create output file
    FILE *outfile = fopen("received_file.txt", "w");
    if (outfile == NULL) {
        perror("Failed to create output file");
        close(sockfd);
        return 0;
    }
    
    // Write HELLO to file
    fprintf(outfile, "%s\n", buffer);
    printf("Received: %s\n", buffer);
    
    // Request words one by one
    while (1) {
        char request[20];
        sprintf(request, "WORD%d", word_count);
        
        // Send word request
        sendto(sockfd, request, strlen(request), 0, (struct sockaddr *)&servaddr, len);
        printf("Requesting: %s\n", request);
        
        // Receive word
        n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *)&servaddr, &len);
        buffer[n] = '\0';
        
        printf("Received word: %s\n", buffer);
        fprintf(outfile, "%s\n", buffer);
        
        if (strcmp(buffer, "FINISH") == 0) {
            break;
        }
        
        word_count++;
    }
    
    fclose(outfile);
    close(sockfd);
    printf("Client terminated. Received file saved as received_file.txt\n");
    return 0;
}