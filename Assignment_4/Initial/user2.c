#include "ksocket.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*
   user2:
     - Attaches to shared memory with user-provided shmid
     - Creates a KTP socket
     - Binds to 127.0.0.1:54321 => sending to 127.0.0.1:12345
     - Tries to receive data from user1
     - Closes & detaches
*/

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[user2] Usage: %s <shmid>\n", argv[0]);
        return 1;
    }
    int shmid = atoi(argv[1]);

    printf("[user2] Attaching to shared memory with ID:%d\n", shmid);
    if (!attach_shared_memory(shmid)) {
        return 1;
    }

    // Create
    int sockfd = k_socket();
    if (sockfd < 0) {
        fprintf(stderr, "[user2] k_socket() failed\n");
        detach_shared_memory(shm);
        return 1;
    }

    // Bind
    if (k_bind(sockfd, "127.0.0.1", 54321, "127.0.0.1", 12345) < 0) {
        fprintf(stderr, "[user2] k_bind() failed\n");
        k_close(sockfd);
        detach_shared_memory(shm);
        return 1;
    }

    // Attempt to receive
    for (int i = 0; i < 5; i++) {
        char buffer[MESSAGE_SIZE];
        int ret = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (ret == ENOMESSAGE) {
            printf("[user2] No data yet, attempt=%d\n", i+1);
            sleep(1);
            continue;
        } else if (ret == ENOTBOUND) {
            fprintf(stderr, "[user2] Socket error: ENOTBOUND\n");
            break;
        } else if (ret < 0) {
            fprintf(stderr, "[user2] k_recvfrom() failed\n");
            break;
        } else {
            // We have data
            printf("[user2] Received: %s\n", buffer);
            break;
        }
    }

    k_close(sockfd);
    detach_shared_memory(shm);
    return 0;
}