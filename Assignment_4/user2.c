// #include "ksocket.h"
// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>

// #define FILE_SIZE (128 * 1024)

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

//     // Receive file
//     char buffer[MESSAGE_SIZE];
//     int bytes_received = 0;
//     int num_messages = FILE_SIZE / MESSAGE_SIZE + (FILE_SIZE % MESSAGE_SIZE ? 1 : 0);
//     while (bytes_received < FILE_SIZE) {
//         int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
//         if (len > 0) {
//             bytes_received += len;
//             printf("[user2] Received %d/%d bytes\n", bytes_received, FILE_SIZE);
//             char reply[32];
//             snprintf(reply, sizeof(reply), "ACK %d", bytes_received);
//             while (k_sendto(sockfd, reply, strlen(reply) + 1, 0) < 0) {
//                 printf("[user2] Waiting for window space...\n");
//                 sleep(1);
//             }
//             printf("[user2] Sent: %s\n", reply);
//         } else {
//             sleep(1);
//         }
//     }

//     // Wait for all messages to be sent
//     int linger = 10;
//     while (linger-- && (shm->sockets[sockfd].send_count > 0 || shm->sockets[sockfd].swnd.size > 0)) {
//         printf("[user2] Lingering... (%d left)\n", linger);
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
    int received = 0;
    const char *replies[] = {"Reply 0 from user2", "Reply 1 from user2", "Reply 2 from user2", "Reply 3 from user2", "Reply 4 from user2"};
    while (received < 5) {
        int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (len > 0) {
            printf("[user2] Received: %s\n", buffer);
            while (k_sendto(sockfd, replies[received], strlen(replies[received]) + 1, 0) < 0) {
                printf("[user2] Waiting for window space...\n");
                sleep(1);
            }
            printf("[user2] Sent: %s\n", replies[received]);
            received++;
        }
        sleep(1);
    }

    k_close(sockfd);
    detach_shared_memory(shm);
    printf("[user2] Done\n");
    return 0;
}