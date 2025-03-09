#include "ksocket.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
   user1:
     - Attaches to shared memory with user-provided shmid
     - Creates a KTP socket
     - Binds to 127.0.0.1:12345 => sending to 127.0.0.1:54321
     - Sends "Hello from user1!"
     - Closes & detaches
*/

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[user1] Usage: %s <shmid>\n", argv[0]);
        return 1;
    }
    int shmid = atoi(argv[1]);
    printf("[user1] Attaching to shared memory with ID:%d\n", shmid);
    if (!attach_shared_memory(shmid)) {
        return 1;
    }

    // Create socket
    int sockfd = k_socket();
    if (sockfd < 0) {
        fprintf(stderr, "[user1] k_socket() failed\n");
        detach_shared_memory(shm);
        return 1;
    }

    // Bind
    if (k_bind(sockfd, "127.0.0.1", 12345, "127.0.0.1", 54321) < 0) {
        fprintf(stderr, "[user1] k_bind() failed\n");
        k_close(sockfd);
        detach_shared_memory(shm);
        return 1;
    }

    // Send message
    const char* msg = "Hello from user1!";
    if (k_sendto(sockfd, msg, strlen(msg) + 1, 0) >= 0) {
        printf("[user1] Sent message: %s\n", msg);
    }

    // Optional wait
    sleep(1);

    k_close(sockfd);
    detach_shared_memory(shm);
    return 0;
}