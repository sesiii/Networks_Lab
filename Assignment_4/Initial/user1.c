#include "ksocket.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <shmid>\n", argv[0]);
        return 1;
    }

    int shmid = atoi(argv[1]); // Get shmid from command-line argument
    printf("Attaching to shared memory with ID: %d\n", shmid);

    // Attach shared memory
    shm = attach_shared_memory(shmid);
    if (shm == NULL) {
        fprintf(stderr, "Failed to attach shared memory\n");
        return 1;
    }

    printf("Shared memory attached successfully\n");

    // Create and bind KTP socket
    int sockfd = k_socket();
    if (sockfd == -1) {
        fprintf(stderr, "Failed to create KTP socket\n");
        return 1;
    }

    printf("KTP socket created successfully with ID: %d\n", sockfd);

    if (k_bind(sockfd, "127.0.0.1", 12345, "127.0.0.1", 54321) == -1) {
        fprintf(stderr, "Failed to bind KTP socket\n");
        return 1;
    }

    printf("KTP socket bound successfully\n");

    // Send a message
    char message[] = "Helu chanpu1!";
    if (k_sendto(sockfd, message, sizeof(message), 0) == -1) {
        fprintf(stderr, "Failed to send message\n");
        return 1;
    }

    printf("Message sent: %s\n", message);

    // Close the socket and cleanup
    k_close(sockfd);
    detach_shared_memory(shm);

    printf("KTP socket closed and shared memory detached\n");

    return 0;
}