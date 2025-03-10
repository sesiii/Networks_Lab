


#include "ksocket.h"
#include <pthread.h>
#include <time.h>
#include <signal.h>

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    (void) dummy;
    keepRunning = 0;
}

int main() {
    srand(time(NULL)); 
    signal(SIGINT, intHandler);

    int shmid = init_shared_memory();
    if (shmid == -1) {
        fprintf(stderr, "[initksocket] Failed to initialize shared memory\n");
        return 1;
    }

    pthread_t thread_r_id, thread_s_id, thread_g_id;
    ThreadArgs args = { shmid };
    if (pthread_create(&thread_r_id, NULL, thread_r, &args) != 0) {
        perror("[initksocket] pthread_create failed for thread_r");
        cleanup_shared_memory(shmid);
        return 1;
    }
    if (pthread_create(&thread_s_id, NULL, thread_s, &args) != 0) {
        perror("[initksocket] pthread_create failed for thread_s");
        pthread_cancel(thread_r_id);
        pthread_join(thread_r_id, NULL);
        cleanup_shared_memory(shmid);
        return 1;
    }
    if (pthread_create(&thread_g_id, NULL, thread_g, &args) != 0) {
        perror("[initksocket] pthread_create failed for thread_g");
        pthread_cancel(thread_r_id);
        pthread_cancel(thread_s_id);
        pthread_join(thread_r_id, NULL);
        pthread_join(thread_s_id, NULL);
        cleanup_shared_memory(shmid);
        return 1;
    }

    printf("[initksocket] Shared memory created with ID: %d\n", shmid);
    printf("[initksocket] Threads R, S, and G started\n");
    printf("[initksocket] Run user1 and user2 with shmid=%d\n", shmid);
    printf("[initksocket] Press Ctrl+C to exit\n");

    while (keepRunning) {
        sleep(1);
    }

    printf("[initksocket] Cleaning up...\n");
    pthread_cancel(thread_r_id);
    pthread_cancel(thread_s_id);
    pthread_cancel(thread_g_id);
    pthread_join(thread_r_id, NULL);
    pthread_join(thread_s_id, NULL);
    pthread_join(thread_g_id, NULL);
    cleanup_shared_memory(shmid);
    return 0;
}