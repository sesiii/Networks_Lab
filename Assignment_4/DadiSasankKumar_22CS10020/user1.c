//Name: Dadi Sasank Kumar
//Roll: 22CS10020
//Assignment 4
//user1.c(Sender)
//This is the sender program that sends messages to the receiver and waits for the ACK.
#include "ksocket.h"
#include <time.h>
#include <sys/select.h>

#define NUM_MESSAGES 100
#define TIMEOUT 2  
#define MAX_RETRANSMISSIONS 5

int main() {
    int sockfd = k_socket(AF_INET, SOCK_KTP, 0);
    if (sockfd < 0) {
        perror("k_socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest_addr.sin_port = htons(8081); 

    char message[512];
    char ack[4];
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    printf("Starting sender...\n\n");
    printf("Starting transmission of %d messages...\n", NUM_MESSAGES);

    srand(time(NULL));

    for (int i = 0; i < NUM_MESSAGES; ++i) {
        snprintf(message, sizeof(message), "Message %d", i+1);

        int retransmissions = 0;
        int ack_received = 0;

        while (retransmissions < MAX_RETRANSMISSIONS && !ack_received) {
            ssize_t len = k_sendto(sockfd, message, sizeof(message), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (len < 0) {
                perror("k_sendto");
                exit(EXIT_FAILURE);
            }
            printf("Sent: %s\n", message);

            // Wait for ACK with timeout
            fd_set readfds;
            struct timeval tv;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            tv.tv_sec = TIMEOUT;
            tv.tv_usec = 0;

            int retval = select(sockfd + 1, &readfds, NULL, NULL, &tv);
            if (retval == -1) {
                perror("select");
                exit(EXIT_FAILURE);
            } else if (retval) {
                // Data is available, read the ACK
                ssize_t ack_len = k_recvfrom(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&src_addr, &addrlen);
                if (ack_len > 0 && strcmp(ack, "ACK") == 0) {
                    printf("Received ACK for message %d\n\n", i+1);
                    ack_received = 1;
                } else {
                    printf("Invalid ACK received for message %d\n", i+1);
                }
            } else {
                printf("Timeout waiting for ACK for message %d, retransmitting...\n", i+1);
                retransmissions++;
            }
        }

        if (!ack_received) {
            printf("Failed to receive ACK for message %d after %d retransmissions, giving up.\n", i+1, MAX_RETRANSMISSIONS);
        }
    }

    k_close(sockfd);
    return 0;
}