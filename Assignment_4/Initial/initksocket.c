#include "ksocket.h"

int main() {
    // Initialize Shared Memory
    int shmid = init_shared_memory();
    if (shmid == -1) {
        fprintf(stderr, "Failed to initialize shared memory\n");
        return 1;
    }

    // Attach Shared Memory
    SharedMemory* shm = attach_shared_memory(shmid);
    if (shm == NULL) {
        fprintf(stderr, "Failed to attach shared memory\n");
        return 1;
    }

    // Initialize KTP Sockets
    for (int i = 0; i < MAX_SOCKETS; i++) {
        shm->sockets[i].is_free = 1; // Mark all sockets as free
    }

    for (int i = 0; i < MAX_SOCKETS; i++) {
        printf("Socket %d: is_free = %d\n", i, shm->sockets[i].is_free);
    }

    printf("Shared memory and KTP sockets initialized successfully\n");

    // Detach Shared Memory
    detach_shared_memory(shm);

    return 0;
}