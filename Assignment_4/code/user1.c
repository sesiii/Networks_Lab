

// #include "ksocket.h"
// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>
// #include <time.h>

// #define CHUNK_SIZE 400
// #define ACK_TIMEOUT 5 // Timeout in seconds

// int main(int argc, char* argv[]) {
//     if (argc < 4) {
//         fprintf(stderr, "[user1] Usage: %s <shmid> <filename> <drop_probability>\n", argv[0]);
//         return 1;
//     }

//     int shmid = atoi(argv[1]);
//     const char* filename = argv[2];
//     float drop_prob = atof(argv[3]);

//     printf("[user1] Attaching to shmid=%d, filename=%s, p=%.2f\n", shmid, filename, drop_prob);
//     shm = attach_shared_memory(shmid);
//     if (shm == NULL) {
//         fprintf(stderr, "[user1] Failed to attach\n");
//         return 1;
//     }
//     shm->drop_probability = drop_prob;

//     FILE* file = fopen(filename, "rb");
//     if (!file) {
//         perror("[user1] Failed to open file");
//         detach_shared_memory(shm);
//         return 1;
//     }

//     int sockfd = k_socket();
//     if (sockfd < 0 || k_bind(sockfd, "127.0.0.1", PORT_USER1, "127.0.0.1", PORT_USER2) < 0) {
//         fclose(file);
//         detach_shared_memory(shm);
//         return 1;
//     }

//     fseek(file, 0, SEEK_END);
//     long file_size = ftell(file);
//     fseek(file, 0, SEEK_SET);

//     char size_msg[32];
//     sprintf(size_msg, "%ld", file_size);
//     while (k_sendto(sockfd, size_msg, strlen(size_msg) + 1, 0) < 0) sleep(1);
//     printf("[user1] Sent file size: %s\n", size_msg);

//     char buffer[CHUNK_SIZE];
//     size_t bytesRead;
//     int total_chunks = 0;
//     int total_transmissions = 0;
//     clock_t start = clock();

//     while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
//         total_chunks++;
//         int retries = 0;
//         while (k_sendto(sockfd, buffer, bytesRead, 0) < 0) {
//             retries++;
//             usleep(100000);
//         }
//         total_transmissions += (1 + retries);
//         if (total_chunks % 100 == 0) {
//             printf("[user1] Sent %d chunks\n", total_chunks);
//         }
//     }

//     while (k_sendto(sockfd, "EOF", 4, 0) < 0) usleep(100000);

//     clock_t end = clock();
//     double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

//     fclose(file);

//     char ack_buffer[MESSAGE_SIZE];
//     time_t start_wait = time(NULL);
//     int received_ack = 0;
//     while (time(NULL) - start_wait < ACK_TIMEOUT) {
//         int len = k_recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0);
//         if (len > 0 && strcmp(ack_buffer, "ACK_EOF") == 0) {
//             received_ack = 1;
//             break;
//         }
//         usleep(100000);
//     }

//     if (!received_ack) {
//         printf("[user1] Warning: Did not receive ACK_EOF within %d seconds\n", ACK_TIMEOUT);
//     } else {
//         printf("[user1] Received ACK_EOF\n");
//     }

//     printf("[user1] Transmission complete: %d chunks, %d transmissions\n", total_chunks, total_transmissions);
//     printf("[user1] Avg transmissions per message: %.2f\n", (float)total_transmissions / total_chunks);
//     printf("[user1] Time taken: %.2f seconds\n", time_taken);
//     printf("[user1] Throughput: %.2f KB/s\n", (file_size / 1024.0) / time_taken);

//     k_close(sockfd);
//     detach_shared_memory(shm);
//     return 0;
// }


#include "ksocket.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define CHUNK_SIZE 400
#define ACK_TIMEOUT 5 

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[user1] Usage: %s <shmid> <filename> [<drop_probability>]\n", argv[0]);
        return 1;
    }

    int shmid = atoi(argv[1]);
    const char* filename = argv[2];
    float drop_prob = P; // Default from ksocket.h
    if (argc >= 4) {
        drop_prob = atof(argv[3]);
    }

    printf("[user1] Attaching to shmid=%d, filename=%s, p=%.2f\n", shmid, filename, drop_prob);
    shm = attach_shared_memory(shmid);
    if (shm == NULL) {
        fprintf(stderr, "[user1] Failed to attach\n");
        return 1;
    }
    shm->drop_probability = drop_prob;

    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("[user1] Failed to open file");
        detach_shared_memory(shm);
        return 1;
    }

    int sockfd = k_socket();
    if (sockfd < 0 || k_bind(sockfd, "127.0.0.1", PORT_USER1, "127.0.0.1", PORT_USER2) < 0) {
        fclose(file);
        detach_shared_memory(shm);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char size_msg[32];
    sprintf(size_msg, "%ld", file_size);
    while (k_sendto(sockfd, size_msg, strlen(size_msg) + 1, 0) < 0) sleep(1);
    printf("[user1] Sent file size: %s\n", size_msg);

    char buffer[CHUNK_SIZE];
    size_t bytesRead;
    int total_chunks = 0;
    int total_transmissions = 0;
    clock_t start = clock();

    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        total_chunks++;
        int retries = 0;
        while (k_sendto(sockfd, buffer, bytesRead, 0) < 0) {
            retries++;
            usleep(100000);
        }
        total_transmissions += (1 + retries);
        if (total_chunks % 100 == 0) {
            printf("[user1] Sent %d chunks\n", total_chunks);
        }
    }

    while (k_sendto(sockfd, "EOF", 4, 0) < 0) usleep(100000);

    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    fclose(file);

    char ack_buffer[MESSAGE_SIZE];
    time_t start_wait = time(NULL);
    int received_ack = 0;
    while (time(NULL) - start_wait < ACK_TIMEOUT) {
        int len = k_recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0);
        if (len > 0 && strcmp(ack_buffer, "ACK_EOF") == 0) {
            received_ack = 1;
            break;
        }
        usleep(100000);
    }

    if (!received_ack) {
        printf("[user1] Warning: Did not receive ACK_EOF within %d seconds\n", ACK_TIMEOUT);
    } else {
        printf("[user1] Received ACK_EOF\n");
    }

    printf("[user1] Transmission complete: %d chunks, %d transmissions\n", total_chunks, total_transmissions);
    printf("[user1] Avg transmissions per message: %.2f\n", (float)total_transmissions / total_chunks);
    printf("[user1] Time taken: %.2f seconds\n", time_taken);
    printf("[user1] Throughput: %.2f KB/s\n", (file_size / 1024.0) / time_taken);

    k_close(sockfd);
    detach_shared_memory(shm);
    return 0;
}