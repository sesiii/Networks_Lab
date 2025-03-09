// #include "ksocket.h"
// #include <pthread.h>
// #include <signal.h>

// static volatile int keepRunning = 1;

// void intHandler(int dummy) {
//     (void) dummy;
//     keepRunning = 0;
// }

// int main(int argc, char *argv[]) {
//     float drop_prob = 0.1; // Default drop probability
//     if (argc > 1) {
//         drop_prob = atof(argv[1]);
//         if (drop_prob < 0 || drop_prob > 1) {
//             fprintf(stderr, "[initksocket] Invalid drop_prob, using 0.1\n");
//             drop_prob = 0.1;
//         }
//     }

//     signal(SIGINT, intHandler);
//     srand(time(NULL)); // Seed for dropMessage()

//     int shmid = init_shared_memory();
//     if (shmid == -1) {
//         fprintf(stderr, "[initksocket] Failed to initialize shared memory\n");
//         return 1;
//     }

//     pthread_t thread_r_id, thread_s_id, gc_id;
//     ThreadArgs args = { shmid, drop_prob };
//     if (pthread_create(&thread_r_id, NULL, thread_r, &args) != 0) {
//         perror("[initksocket] pthread_create failed for thread_r");
//         cleanup_shared_memory(shmid);
//         return 1;
//     }
//     if (pthread_create(&thread_s_id, NULL, thread_s, &args) != 0) {
//         perror("[initksocket] pthread_create failed for thread_s");
//         pthread_cancel(thread_r_id);
//         pthread_join(thread_r_id, NULL);
//         cleanup_shared_memory(shmid);
//         return 1;
//     }
//     if (pthread_create(&gc_id, NULL, garbage_collector, &shmid) != 0) {
//         perror("[initksocket] pthread_create failed for garbage_collector");
//         pthread_cancel(thread_r_id);
//         pthread_cancel(thread_s_id);
//         pthread_join(thread_r_id, NULL);
//         pthread_join(thread_s_id, NULL);
//         cleanup_shared_memory(shmid);
//         return 1;
//     }

//     printf("[initksocket] Shared memory created with ID: %d\n", shmid);
//     printf("[initksocket] Threads R, S, and GC started with drop_prob=%.2f\n", drop_prob);
//     printf("[initksocket] Run user1 and user2 with shmid=%d\n", shmid);
//     printf("[initksocket] Press Ctrl+C to exit\n");

//     while (keepRunning) {
//         sleep(1);
//     }

//     printf("[initksocket] Cleaning up...\n");
//     pthread_cancel(thread_r_id);
//     pthread_cancel(thread_s_id);
//     pthread_cancel(gc_id);
//     pthread_join(thread_r_id, NULL);
//     pthread_join(thread_s_id, NULL);
//     pthread_join(gc_id, NULL);
//     cleanup_shared_memory(shmid);
//     return 0;
// }


#include "ksocket.h"
#include <pthread.h>
#include <signal.h>

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    (void) dummy;
    keepRunning = 0;
}

int main(int argc, char *argv[]) {
    float drop_prob = 0.1; // Default drop probability
    if (argc > 1) {
        drop_prob = atof(argv[1]);
        if (drop_prob < 0 || drop_prob > 1) {
            fprintf(stderr, "[initksocket] Invalid drop_prob, using 0.1\n");
            drop_prob = 0.1;
        }
    }

    signal(SIGINT, intHandler);
    srand(time(NULL)); // Seed for dropMessage()

    int shmid = init_shared_memory();
    if (shmid == -1) {
        fprintf(stderr, "[initksocket] Failed to initialize shared memory\n");
        return 1;
    }

    pthread_t thread_r_id;
    ThreadArgs args = { shmid, drop_prob };
    if (pthread_create(&thread_r_id, NULL, thread_r, &args) != 0) {
        perror("[initksocket] pthread_create failed for thread_r");
        cleanup_shared_memory(shmid);
        return 1;
    }

    printf("[initksocket] Shared memory created with ID: %d\n", shmid);
    printf("[initksocket] Thread R started with drop_prob=%.2f\n", drop_prob);
    printf("[initksocket] Run user1 and user2 with shmid=%d\n", shmid);
    printf("[initksocket] Press Ctrl+C to exit\n");

    while (keepRunning) {
        sleep(1);
    }

    printf("[initksocket] Cleaning up...\n");
    pthread_cancel(thread_r_id);
    pthread_join(thread_r_id, NULL);
    cleanup_shared_memory(shmid);
    return 0;
}