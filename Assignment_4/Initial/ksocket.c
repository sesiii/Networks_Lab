#include "ksocket.h"

// Define shm as a global variable
SharedMemory* shm;

// Initialize Shared Memory
int init_shared_memory() {
    printf("Initializing shared memory...\n");

    int shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget failed");
        return -1;
    }

    printf("Shared memory created with ID: %d\n", shmid);

    shm = attach_shared_memory(shmid);
    if (shm == NULL) {
        return -1;
    }

    printf("Shared memory attached successfully\n");

    // Initialize all sockets as free
    for (int i = 0; i < MAX_SOCKETS; i++) {
        shm->sockets[i].is_free = 1;
    }

    printf("Shared memory initialized successfully\n");
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

// Create a KTP Socket
int k_socket() {
    printf("Attempting to create a KTP socket...\n");

    if (shm == NULL) {
        fprintf(stderr, "Shared memory is not attached!\n");
        return -1;
    }

    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (shm->sockets[i].is_free) {
            printf("Found free slot at index %d\n", i);

            shm->sockets[i].is_free = 0; // Mark as allotted
            shm->sockets[i].udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if (shm->sockets[i].udp_socket == -1) {
                perror("socket creation failed");
                return -1;
            }

            shm->sockets[i].process_id = getpid();
            printf("KTP socket created successfully at index %d\n", i);
            return i; // Return the socket ID
        }
    }

    printf("No free slots available in shared memory\n");
    return ENOSPACE; // No space available
}

// Bind a KTP Socket
int k_bind(int sockfd, const char* src_ip, int src_port, const char* dest_ip, int dest_port) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        return ENOTBOUND; // Invalid socket
    }

    // Bind source IP/Port
    shm->sockets[sockfd].src_addr.sin_family = AF_INET;
    shm->sockets[sockfd].src_addr.sin_port = htons(src_port);
    inet_pton(AF_INET, src_ip, &shm->sockets[sockfd].src_addr.sin_addr);

    // Set destination IP/Port
    shm->sockets[sockfd].dest_addr.sin_family = AF_INET;
    shm->sockets[sockfd].dest_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip, &shm->sockets[sockfd].dest_addr.sin_addr);

    // Bind the UDP socket
    if (bind(shm->sockets[sockfd].udp_socket, (struct sockaddr*) &shm->sockets[sockfd].src_addr, sizeof(shm->sockets[sockfd].src_addr)) == -1) {
        perror("bind failed");
        return ENOTBOUND;
    }

    return 0; // Success
}

// Send a Message
int k_sendto(int sockfd, const void* buf, size_t len, int flags) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        return ENOTBOUND; // Invalid socket
    }

    if (len > MESSAGE_SIZE) {
        return -1; // Message too large
    }

    // Send the message using UDP
    if (sendto(shm->sockets[sockfd].udp_socket, buf, len, flags, (struct sockaddr*) &shm->sockets[sockfd].dest_addr, sizeof(shm->sockets[sockfd].dest_addr)) == -1) {
        perror("sendto failed");
        return -1;
    }

    return len; // Return the number of bytes sent
}

// Receive a Message
int k_recvfrom(int sockfd, void* buf, size_t len, int flags) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        return ENOTBOUND; // Invalid socket
    }

    // Receive the message using UDP
    socklen_t addr_len = sizeof(shm->sockets[sockfd].src_addr);
    int recv_len = recvfrom(shm->sockets[sockfd].udp_socket, buf, len, flags, (struct sockaddr*) &shm->sockets[sockfd].src_addr, &addr_len);
    if (recv_len == -1) {
        perror("recvfrom failed");
        return ENOMESSAGE;
    }

    return recv_len; // Return the number of bytes received
}

// Close a KTP Socket
int k_close(int sockfd) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        return ENOTBOUND; // Invalid socket
    }

    close(shm->sockets[sockfd].udp_socket);
    shm->sockets[sockfd].is_free = 1; // Mark as free
    return 0; // Success
}