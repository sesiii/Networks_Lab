#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>

#define SHM_SIZE 1024

// Shared memory structure with semaphore
typedef struct {
    sem_t lock;
    char data[SHM_SIZE];
} shared_buffer;

shared_buffer *init_shared_memory() {
    // Create shared memory object
    shared_buffer *shm = mmap(NULL, sizeof(shared_buffer), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&shm->lock, 1, 1); // 1 means shared between processes, initial value 1
    return shm;
}

void write_to_shared(shared_buffer *shm, const char *message) {
    sem_wait(&shm->lock); // Lock
    strncpy(shm->data, message, SHM_SIZE - 1);
    shm->data[SHM_SIZE - 1] = '\0';
    sem_post(&shm->lock); // Unlock
}

void read_from_shared(shared_buffer *shm, char *buffer) {
    sem_wait(&shm->lock);
    strcpy(buffer, shm->data);
    sem_post(&shm->lock);
}

// Example usage
int main() {
    shared_buffer *shm = init_shared_memory();
    write_to_shared(shm, "Hello Shared Memory");
    char buffer[SHM_SIZE];
    read_from_shared(shm, buffer);
    printf("Read: %s\n", buffer);
    sem_destroy(&shm->lock);
    munmap(shm, sizeof(shared_buffer));
    return 0;
}