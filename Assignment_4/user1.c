// #include "ksocket.h"
// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>

// #define FILE_SIZE (128 * 1024) // 128 KB

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

//     // Generate a 128 KB file
//     char *data = malloc(FILE_SIZE);
//     for (int i = 0; i < FILE_SIZE; i++) {
//         data[i] = 'A' + (i % 26);
//     }
//     int num_messages = FILE_SIZE / MESSAGE_SIZE + (FILE_SIZE % MESSAGE_SIZE ? 1 : 0);

//     // Send file
//     int bytes_sent = 0;
//     for (int i = 0; i < num_messages; i++) {
//         int offset = i * MESSAGE_SIZE;
//         int len = (i == num_messages - 1 && FILE_SIZE % MESSAGE_SIZE) ? FILE_SIZE % MESSAGE_SIZE : MESSAGE_SIZE;
//         while (k_sendto(sockfd, data + offset, len, 0) < 0) {
//             printf("[user1] Waiting for window space... (seq=%d)\n", i);
//             sleep(1);
//         }
//         bytes_sent += len;
//         printf("[user1] Sent %d/%d bytes (seq=%d)\n", bytes_sent, FILE_SIZE, i);
//     }

//     // Receive acknowledgment or response
//     char buffer[MESSAGE_SIZE];
//     int received = 0;
//     while (received < num_messages) {
//         int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
//         if (len > 0) {
//             printf("[user1] Received: %s (len=%d)\n", buffer, len);
//             received++;
//         } else {
//             sleep(1);
//         }
//     }

//     free(data);
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

    const char *messages[] = {"Msg 0 from user1", "Msg 1 from user1", "Msg 2 from user1", "Msg 3 from user1", "Msg 4 from user1"};
    int num_messages = 5;
    for (int i = 0; i < num_messages; i++) {
        while (k_sendto(sockfd, messages[i], strlen(messages[i]) + 1, 0) < 0) {
            printf("[user1] Waiting for window space...\n");
            sleep(1);
        }
        printf("[user1] Sent: %s\n", messages[i]);
    }

    char buffer[MESSAGE_SIZE];
    int received = 0;
    while (received < num_messages) {
        int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (len > 0) {
            printf("[user1] Received: %s\n", buffer);
            received++;
        }
        sleep(1);
    }

    k_close(sockfd);
    detach_shared_memory(shm);
    printf("[user1] Done\n");
    return 0;
}