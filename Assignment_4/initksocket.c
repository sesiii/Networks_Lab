#include "ksocket.h"

void *receive_thread(void *arg) {
    int sockfd = *(int *)arg;
    char buffer[512];
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    bool received[MAX_SEQ_NUM] = {false}; // Track received messages
    int expected_seq_num = 1; // Start with sequence number 1

    while (1) {
        ssize_t len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src_addr, &addrlen);
        if (len > 0) {
            int seq_num;
            sscanf(buffer, "Message %d", &seq_num);

            if (seq_num == expected_seq_num) {
                printf("Received: %s\n", buffer);
                received[seq_num % MAX_SEQ_NUM] = true;
                expected_seq_num = (expected_seq_num % MAX_SEQ_NUM) + 1;
            } else if (received[seq_num % MAX_SEQ_NUM]) {
                printf("Duplicate message received: %s\n", buffer);
            } else {
                printf("Out-of-order message received: %s\n", buffer);
                received[seq_num % MAX_SEQ_NUM] = true;
            }

            // Send ACK
            char ack[512];
            snprintf(ack, sizeof(ack), "ACK %d", seq_num);
            k_sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&src_addr, addrlen);
            printf("Sent ACK for message %d\n", seq_num);
        }
    }
    return NULL;
}

int main() {
    int sockfd = k_socket(AF_INET, SOCK_KTP, 0);
    if (sockfd < 0) {
        perror("k_socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (k_bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("k_bind");
        exit(EXIT_FAILURE);
    }

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_thread, &sockfd);

    pthread_join(recv_thread, NULL);
    k_close(sockfd);

    return 0;
}