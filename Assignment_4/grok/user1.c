// #include "ksocket.h"
// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>

// int main(int argc, char* argv[]) {
//     if (argc != 2) {
//         fprintf(stderr, "[user1] Usage: %s <shmid>\n", argv[0]);
//         return 1;
//     }

//     int shmid = atoi(argv[1]);
//     printf("[user1] Attaching to shmid=%d\n", shmid);
//     shm = attach_shared_memory(shmid);
//     if (shm == NULL) {
//         fprintf(stderr, "[user1] Failed to attach\n");
//         return 1;
//     }

//     int sockfd = k_socket();
//     if (sockfd < 0) {
//         fprintf(stderr, "[user1] k_socket failed\n");
//         detach_shared_memory(shm);
//         return 1;
//     }

//     if (k_bind(sockfd, "127.0.0.1", PORT_USER1, "127.0.0.1", PORT_USER2) < 0) {
//         fprintf(stderr, "[user1] k_bind failed\n");
//         k_close(sockfd);
//         detach_shared_memory(shm);
//         return 1;
//     }

//     const char *messages[] = {"Hello 1 from user1!", "Hello 2 from user1!", "Hello 3 from user1!"};
//     int num_messages = 3;
//     for (int i = 0; i < num_messages; i++) {
//         while (k_sendto(sockfd, messages[i], strlen(messages[i]) + 1, 0) < 0) {
//             printf("[user1] Waiting for window space...\n");
//             sleep(1);
//         }
//         printf("[user1] Sent: %s\n", messages[i]);
//     }

//     char buffer[MESSAGE_SIZE];
//     int attempts = 15; // Increased to ensure all replies are received
//     int received = 0;
//     while (received < num_messages && attempts--) {
//         int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
//         if (len > 0) {
//             printf("[user1] Received: %s\n", buffer);
//             received++;
//         } else {
//             printf("[user1] Waiting for reply... (%d left)\n", attempts);
//             sleep(1);
//         }
//     }

//     k_close(sockfd);
//     detach_shared_memory(shm);
//     printf("[user1] Done\n");
//     return 0;
// }


#include "ksocket.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[user1] Usage: %s <shmid>\n", argv[0]);
        return 1;
    }

    int shmid = atoi(argv[1]);
    printf("[user1] Attaching to shmid=%d\n", shmid);
    shm = attach_shared_memory(shmid);
    if (shm == NULL) {
        fprintf(stderr, "[user1] Failed to attach\n");
        return 1;
    }

    int sockfd = k_socket();
    if (sockfd < 0) {
        fprintf(stderr, "[user1] k_socket failed\n");
        detach_shared_memory(shm);
        return 1;
    }

    if (k_bind(sockfd, "127.0.0.1", PORT_USER1, "127.0.0.1", PORT_USER2) < 0) {
        fprintf(stderr, "[user1] k_bind failed\n");
        k_close(sockfd);
        detach_shared_memory(shm);
        return 1;
    }

    char messages[10][50];
    for (int i = 0; i < 10; i++) {
        snprintf(messages[i], sizeof(messages[i]), "Hello %d from user1!", i + 1);
    }
    int num_messages = 10;
    for (int i = 0; i < num_messages; i++) {
        while (k_sendto(sockfd, messages[i], strlen(messages[i]) + 1, 0) < 0) {
            printf("[user1] Waiting for window space...\n");
            sleep(1);
        }
        printf("[user1] Sent: %s\n", messages[i]);
    }

    char buffer[MESSAGE_SIZE];
    int received = 0;
    int total_attempts = 60; // Increased timeout
    while (received < num_messages && total_attempts > 0) {
        int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (len > 0) {
            printf("[user1] Received: %s\n", buffer);
            received++;
            total_attempts = 60; // Reset timeout on receipt
        } else {
            printf("[user1] Waiting for reply... (%d left)\n", total_attempts);
            sleep(1);
            total_attempts--;
        }
    }

    if (received < num_messages) {
        printf("[user1] Timed out, received only %d/%d messages\n", received, num_messages);
    }

    k_close(sockfd);
    detach_shared_memory(shm);
    printf("[user1] Done\n");
    return 0;
}