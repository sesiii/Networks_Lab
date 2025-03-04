#include "ksocket.h"
#include <sys/select.h>

typedef struct {
    int sockfd;
    struct sockaddr_in dest_addr;
    socklen_t addrlen;
    char message[512];
    int seq_num;
    int retransmissions;
    int ack_received;
} KTP_Message;

KTP_Message ktp_message;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int k_socket(int domain, int type, int protocol) {
    if (type != SOCK_KTP) {
        errno = EINVAL;
        return -1;
    }
    // Create a UDP socket
    int sockfd = socket(domain, SOCK_DGRAM, protocol);
    if (sockfd < 0) {
        return -1;
    }
    return sockfd;
}

int k_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

ssize_t k_sendto(int sockfd, const void *buf, size_t len, int flags,
                 const struct sockaddr *dest_addr, socklen_t addrlen) {
    pthread_mutex_lock(&lock);
    memset(&ktp_message, 0, sizeof(KTP_Message));
    ktp_message.sockfd = sockfd;
    ktp_message.dest_addr = *(struct sockaddr_in *)dest_addr;
    ktp_message.addrlen = addrlen;
    strncpy(ktp_message.message, buf, len);
    ktp_message.seq_num = (ktp_message.seq_num % MAX_SEQ_NUM) + 1;
    ktp_message.retransmissions = 0;
    ktp_message.ack_received = 0;

    while (ktp_message.retransmissions < MAX_RETRANSMISSIONS && !ktp_message.ack_received) {
        ssize_t sent_len = sendto(sockfd, ktp_message.message, len, flags, dest_addr, addrlen);
        if (sent_len < 0) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
        printf("Sent message %d: %s\n", ktp_message.seq_num, ktp_message.message);

        // Wait for ACK with timeout
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;

        int retval = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (retval == -1) {
            pthread_mutex_unlock(&lock);
            return -1;
        } else if (retval) {
            // Data is available, read the ACK
            char ack[512];
            struct sockaddr_in src_addr;
            socklen_t src_addrlen = sizeof(src_addr);
            ssize_t ack_len = recvfrom(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&src_addr, &src_addrlen);
            if (ack_len > 0) {
                int ack_seq_num;
                sscanf(ack, "ACK %d", &ack_seq_num);
                if (ack_seq_num == ktp_message.seq_num) {
                    printf("Received ACK for message %d\n", ktp_message.seq_num);
                    ktp_message.ack_received = 1;
                } else {
                    printf("Invalid ACK received for message %d\n", ktp_message.seq_num);
                }
            }
        } else {
            printf("Timeout waiting for ACK for message %d, retransmitting...\n", ktp_message.seq_num);
            ktp_message.retransmissions++;
        }
    }

    if (!ktp_message.ack_received) {
        printf("Failed to receive ACK for message %d after %d retransmissions, giving up.\n", ktp_message.seq_num, MAX_RETRANSMISSIONS);
        pthread_mutex_unlock(&lock);
        return -1;
    }

    pthread_mutex_unlock(&lock);
    return len;
}

ssize_t k_recvfrom(int sockfd, void *buf, size_t len, int flags,
                   struct sockaddr *src_addr, socklen_t *addrlen) {
    return recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

int k_close(int sockfd) {
    return close(sockfd);
}

int dropMessage(float p) {
    float r = (float)rand() / RAND_MAX;
    return r < p;
}