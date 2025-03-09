// #include "ksocket.h"
// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>

// int main(int argc, char* argv[]) {
//     if (argc != 2) {
//         fprintf(stderr, "[user2] Usage: %s <shmid>\n", argv[0]);
//         return 1;
//     }

//     int shmid = atoi(argv[1]);
//     printf("[user2] Attaching to shmid=%d\n", shmid);
//     shm = attach_shared_memory(shmid);
//     if (shm == NULL) {
//         fprintf(stderr, "[user2] Failed to attach\n");
//         return 1;
//     }

//     int sockfd = k_socket();
//     if (sockfd < 0) {
//         fprintf(stderr, "[user2] k_socket failed\n");
//         detach_shared_memory(shm);
//         return 1;
//     }

//     if (k_bind(sockfd, "127.0.0.1", PORT_USER2, "127.0.0.1", PORT_USER1) < 0) {
//         fprintf(stderr, "[user2] k_bind failed\n");
//         k_close(sockfd);
//         detach_shared_memory(shm);
//         return 1;
//     }

//     char buffer[MESSAGE_SIZE];
//     int attempts = 15;
//     int received = 0;
//     const char *replies[] = {"Hi 1 from user2!", "Hi 2 from user2!", "Hi 3 from user2!"};
//     while (received < 3 && attempts--) {
//         int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
//         if (len > 0) {
//             printf("[user2] Received: %s\n", buffer);
//             while (k_sendto(sockfd, replies[received], strlen(replies[received]) + 1, 0) < 0) {
//                 printf("[user2] Waiting for window space...\n");
//                 sleep(1);
//             }
//             printf("[user2] Sent: %s\n", replies[received]);
//             received++;
//         } else {
//             printf("[user2] Waiting... (%d left)\n", attempts);
//             sleep(1);
//         }
//     }

//     // Wait for replies to be sent and acknowledged
//     int linger = 5; // Wait 5 seconds to ensure replies are processed
//     while (linger-- && (shm->sockets[sockfd].send_count > 0 || shm->sockets[sockfd].swnd.size > 0)) {
//         printf("[user2] Lingering to ensure replies are sent... (%d left)\n", linger);
//         sleep(1);
//     }

//     k_close(sockfd);
//     detach_shared_memory(shm);
//     printf("[user2] Done\n");
//     return 0;
// }


#include "ksocket.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[user2] Usage: %s <shmid>\n", argv[0]);
        return 1;
    }

    int shmid = atoi(argv[1]);
    printf("[user2] Attaching to shmid=%d\n", shmid);
    shm = attach_shared_memory(shmid);
    if (shm == NULL) {
        fprintf(stderr, "[user2] Failed to attach\n");
        return 1;
    }

    int sockfd = k_socket();
    if (sockfd < 0) {
        fprintf(stderr, "[user2] k_socket failed\n");
        detach_shared_memory(shm);
        return 1;
    }

    if (k_bind(sockfd, "127.0.0.1", PORT_USER2, "127.0.0.1", PORT_USER1) < 0) {
        fprintf(stderr, "[user2] k_bind failed\n");
        k_close(sockfd);
        detach_shared_memory(shm);
        return 1;
    }

    char buffer[MESSAGE_SIZE];
    int attempts = 30; // Increased for more messages
    int received = 0;
    const char *replies[100];
    for (int i = 0; i < 30; i++) {
        char message[50];
        snprintf(message, sizeof(message), "Hi %d from user2!", i + 1);
        replies[i] = strdup(message);
    }
    while (received < 10 && attempts--) {
        int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (len > 0) {
            printf("[user2] Received: %s\n", buffer);
            while (k_sendto(sockfd, replies[received], strlen(replies[received]) + 1, 0) < 0) {
                printf("[user2] Waiting for window space...\n");
                sleep(1);
            }
            printf("[user2] Sent: %s\n", replies[received]);
            received++;
        } else {
            printf("[user2] Waiting... (%d left)\n", attempts);
            sleep(1);
        }
    }

    int linger = 5;
    while (linger-- && (shm->sockets[sockfd].send_count > 0 || shm->sockets[sockfd].swnd.size > 0)) {
        printf("[user2] Lingering to ensure replies are sent... (%d left)\n", linger);
        sleep(1);
    }

    k_close(sockfd);
    detach_shared_memory(shm);
    printf("[user2] Done\n");
    return 0;
}