#include "ksocket.h"
#include <stdio.h>

int main() {
    printf("Initializing shared memory...\n");

    // Initialize shared memory
    int shmid = init_shared_memory();
    if (shmid == -1) {
        fprintf(stderr, "Failed to initialize shared memory\n");
        return 1;
    }

    printf("Shared memory created with ID: %d\n", shmid);

    // Keep the program running so the shared memory is not deleted
    printf("Shared memory is ready. Run user1 and user2 with shmid = %d\n", shmid);
    printf("Press Ctrl+C to exit and cleanup shared memory.\n");

    while (1) {
        sleep(1); // Keep the program running
    }

    return 0;
}