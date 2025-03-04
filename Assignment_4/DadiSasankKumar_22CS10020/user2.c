//Name: Dadi Sasank Kumar
//Roll: 22CS10020
//Assignment 4
//user2.c(Receiver)
//This is the receiver program that receives messages from the sender and sends ACK.

#include "ksocket.h"
#include <stdbool.h>

#define NUM_MESSAGES 100
#define MAX_SEQ_NUM 256 // 8-bit sequence number

bool received[MAX_SEQ_NUM] = {false}; // Track received messages

int main() {
    int sockfd = k_socket(AF_INET, SOCK_KTP, 0);
    if (sockfd < 0) {
        perror("k_socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8081); 

    if (k_bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("k_bind");
        exit(EXIT_FAILURE);
    }

    char buffer[512];
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    int received_messages = 0;

    printf("Starting receiver...\n\n");
    while (received_messages < NUM_MESSAGES) {
        ssize_t len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src_addr, &addrlen);
        if (len > 0) {
            int seq_num;
            sscanf(buffer, "Message %d", &seq_num);

            if (!received[seq_num % MAX_SEQ_NUM]) {
                printf("Received: %s\n", buffer);
                received[seq_num % MAX_SEQ_NUM] = true;
                received_messages++;
            } else {
                printf("Duplicate message received: %s\n", buffer);
            }

            // Send ACK
            char ack[] = "ACK";
            k_sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&src_addr, addrlen);
            printf("Sent ACK for message %d\n\n", seq_num);
        }
    }

    k_close(sockfd);
    return 0;
}