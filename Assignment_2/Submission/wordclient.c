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
    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr;
    socklen_t len;

    
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //internal ip
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_family = AF_INET;  //ipv_4
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Get file name from user
    char filename[MAXLINE];
    printf("Enter the file name: ");
    scanf("%s", filename);

    sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("Sent file request for: %s\n", filename);

    len = sizeof(servaddr);

    // Receive response 
    int n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *)&servaddr, &len);
    buffer[n] = '\0';

    if (strncmp(buffer, "NOTFOUND", 8) == 0) {
        printf("FILE NOT FOUND\n");
        close(sockfd);
        return 0;
    }

    // Create new file to store received words
    FILE *outfile = fopen("received_file.txt", "w");
    if (outfile == NULL) {
        perror("Failed to create output file");
        close(sockfd);
        return 0;
    }

    // Write HELLO to the new file
    fprintf(outfile, "%s\n", buffer);

    // Request and receive words from server
    while (1) {
        sendto(sockfd, "NEXTWORD", strlen("NEXTWORD"), 0, (struct sockaddr *)&servaddr, len);
        n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *)&servaddr, &len);
        buffer[n] = '\0';
        fprintf(outfile, "%s\n", buffer);
        printf("Received word: %s\n", buffer);

        if (strcmp(buffer, "FINISH") == 0) {
            break;
        }
    }

    fclose(outfile);
    close(sockfd);
    printf("Client terminated. Received file saved as received_file.txt\n");

    return 0;
}