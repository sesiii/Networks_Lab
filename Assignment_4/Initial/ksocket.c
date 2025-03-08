#include "ksocket.h"

// Initialize Shared Memory
int init_shared_memory() {
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget failed");
        return -1;
    }
    return shmid;
}

// Attach Shared Memory
SharedMemory* attach_shared_memory(int shmid) {
    SharedMemory* shm = (SharedMemory*) shmat(shmid, NULL, 0);
    if (shm == (void*) -1) {
        perror("shmat failed");
        return NULL;
    }
    return shm;
}

// Detach Shared Memory
void detach_shared_memory(SharedMemory* shm) {
    if (shmdt(shm) == -1) {
        perror("shmdt failed");
    }
}

// Cleanup Shared Memory
void cleanup_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
    }
}