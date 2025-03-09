#include "ksocket.h"
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

static volatile int keepRunning = 1;

// Ctrl+C handler
void intHandler(int dummy) {
    (void)dummy;
    keepRunning = 0;
}

int main() {
    signal(SIGINT, intHandler);

    printf("[initksocket] Initializing shared memory...\n");
    int shmid = init_shared_memory();
    if (shmid < 0) {
        fprintf(stderr, "[initksocket] Failed to init shared mem\n");
        return 1;
    }
    printf("[initksocket] Shared memory created with ID: %d\n", shmid);

    // Spawn thread R
    pthread_t thr;
    ThreadRArgs tra;
    tra.shmid = shmid;

    printf("[initksocket] Starting Thread R...\n");
    if (pthread_create(&thr, NULL, thread_r, &tra) != 0) {
        perror("[initksocket] pthread_create failed");
        cleanup_shared_memory(shmid);
        return 1;
    }
    printf("[initksocket] Thread R started\n");
    printf("[initksocket] Shared memory is ready. Run user1 and user2 with shmid=%d.\n", shmid);
    printf("[initksocket] Press Ctrl+C to stop.\n");

    // Wait for Ctrl+C
    while (keepRunning) {
        sleep(1);
    }

    printf("[initksocket] Caught Ctrl+C, cleaning up...\n");
    cleanup_shared_memory(shmid);
    return 0;
}