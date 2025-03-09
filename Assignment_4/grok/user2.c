


#include "ksocket.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define CHUNK_SIZE 400

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[user2] Usage: %s <shmid> <output_filename>\n", argv[0]);
        return 1;
    }

    int shmid = atoi(argv[1]);
    const char* filename = argv[2];

    printf("[user2] Attaching to shmid=%d, output=%s\n", shmid, filename);
    shm = attach_shared_memory(shmid);
    if (shm == NULL) return 1;

    int sockfd = k_socket();
    if (sockfd < 0 || k_bind(sockfd, "127.0.0.1", PORT_USER2, "127.0.0.1", PORT_USER1) < 0) {
        detach_shared_memory(shm);
        return 1;
    }

    char buffer[MESSAGE_SIZE];
    while (1) {
        int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (len > 0) break;
        sleep(1);
    }
    long expected_size = atol(buffer);
    printf("[user2] Expected file size: %ld bytes\n", expected_size);

    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("[user2] Failed to open output file");
        k_close(sockfd);
        detach_shared_memory(shm);
        return 1;
    }

    long total_received = 0;
    int chunks_received = 0;
    clock_t start = clock();

    while (total_received < expected_size) {
        int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (len > 0) {
            if (strcmp(buffer, "EOF") == 0) break;
            fwrite(buffer, 1, len, file);
            total_received += len;
            chunks_received++;
            if (chunks_received % 100 == 0) {
                printf("[user2] Received %ld bytes (%d chunks)\n", total_received, chunks_received);
            }
        }
        usleep(1000);
    }

    fclose(file);
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("[user2] Received: %ld bytes in %d chunks\n", total_received, chunks_received);
    printf("[user2] Time taken: %.2f seconds\n", time_taken);
    printf("[user2] Throughput: %.2f KB/s\n", (total_received / 1024.0) / time_taken);

    int retries = 0;
    while (k_sendto(sockfd, "ACK_EOF", 8, 0) < 0 && retries < 10) {
        retries++;
        sleep(1);
    }
    if (retries >= 10) {
        printf("[user2] Warning: Failed to send ACK_EOF after %d retries\n", retries);
    } else {
        printf("[user2] Sent ACK_EOF\n");
    }

    k_close(sockfd);
    detach_shared_memory(shm);
    return 0;
}