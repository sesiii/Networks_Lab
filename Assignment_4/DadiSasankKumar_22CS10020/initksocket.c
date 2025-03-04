//Name: Dadi Sasank Kumar
//Roll: 22CS10020
//Assignment 4
//initksocket.c
#include "ksocket.h"

void *receive_thread(void *arg) {
    int sockfd = *(int *)arg;
    char buffer[512];
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);

    while (1) {
        ssize_t len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src_addr, &addrlen);
        if (len > 0) {
            printf("Received: %s\n", buffer);
            // Send ACK
            char ack[] = "ACK";
            k_sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&src_addr, addrlen);
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