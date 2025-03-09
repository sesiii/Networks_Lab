#include "ksocket.h"
#include <sys/select.h>

SharedMemory* shm = NULL;

/*--------------------------------------------------
  SHARED MEMORY
--------------------------------------------------*/
int init_shared_memory() {
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("[init_shared_memory] shmget failed");
        return -1;
    }

    SharedMemory* ptr = attach_shared_memory(shmid);
    if (!ptr) {
        return -1;
    }

    // Initialize the array of sockets
    for (int i = 0; i < MAX_SOCKETS; i++) {
        ptr->sockets[i].is_free = 1;
        ptr->sockets[i].udp_fd = -1;
        ptr->sockets[i].creator_pid = 0;
        memset(&ptr->sockets[i].src_addr, 0, sizeof(ptr->sockets[i].src_addr));
        memset(&ptr->sockets[i].dst_addr, 0, sizeof(ptr->sockets[i].dst_addr));
        memset(ptr->sockets[i].recv_buffer, 0, MESSAGE_SIZE);
        memset(ptr->sockets[i].send_buffer, 0, MESSAGE_SIZE);
        ptr->sockets[i].has_data = 0;
    }

    return shmid;
}

SharedMemory* attach_shared_memory(int shmid) {
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        perror("[attach_shared_memory] shmat failed");
        return NULL;
    }
    shm = (SharedMemory*)addr;
    return shm;
}

void detach_shared_memory(SharedMemory* ptr) {
    if (shmdt(ptr) == -1) {
        perror("[detach_shared_memory] shmdt failed");
    } else {
        printf("[detach_shared_memory] Detached shared memory successfully\n");
    }
}

void cleanup_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("[cleanup_shared_memory] shmctl IPC_RMID failed");
    } else {
        printf("[cleanup_shared_memory] Shared memory segment %d removed\n", shmid);
    }
}

/*--------------------------------------------------
  K_SOCKET API
--------------------------------------------------*/
int k_socket() {
    if (!shm) {
        fprintf(stderr, "[k_socket] Shared memory not attached\n");
        return -1;
    }
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (shm->sockets[i].is_free) {
            shm->sockets[i].is_free = 0;
            shm->sockets[i].udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (shm->sockets[i].udp_fd < 0) {
                perror("[k_socket] socket() failed");
                shm->sockets[i].is_free = 1;
                return -1;
            }
            shm->sockets[i].creator_pid = getpid();
            printf("[k_socket] Created socket at index=%d (fd=%d)\n", i, shm->sockets[i].udp_fd);
            return i;
        }
    }
    return ENOSPACE;
}

int k_bind(int sockfd, const char* src_ip, int src_port, const char* dst_ip, int dst_port) {
    if (!shm || sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        fprintf(stderr, "[k_bind] Invalid socket or not allotted\n");
        return ENOTBOUND;
    }
    KTP_Socket* socket_ptr = &shm->sockets[sockfd];

    socket_ptr->src_addr.sin_family = AF_INET;
    socket_ptr->src_addr.sin_port = htons(src_port);
    inet_pton(AF_INET, src_ip, &socket_ptr->src_addr.sin_addr);
    if (bind(socket_ptr->udp_fd, (struct sockaddr*)&socket_ptr->src_addr, sizeof(socket_ptr->src_addr)) < 0) {
        perror("[k_bind] bind() failed");
        return ENOTBOUND;
    }

    socket_ptr->dst_addr.sin_family = AF_INET;
    socket_ptr->dst_addr.sin_port = htons(dst_port);
    inet_pton(AF_INET, dst_ip, &socket_ptr->dst_addr.sin_addr);

    printf("[k_bind] Socket %d bound to (%s:%d), dst=(%s:%d)\n", 
           sockfd, src_ip, src_port, dst_ip, dst_port);
    return 0;
}

int k_sendto(int sockfd, const void* buf, size_t len, int flags) {
    if (!shm || sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        fprintf(stderr, "[k_sendto] Invalid socket\n");
        return ENOTBOUND;
    }
    if (len > MESSAGE_SIZE) {
        fprintf(stderr, "[k_sendto] message size is too large\n");
        return -1;
    }
    KTP_Socket* s = &shm->sockets[sockfd];

    int ret = sendto(s->udp_fd, buf, len, flags,
                     (struct sockaddr*)&s->dst_addr, sizeof(s->dst_addr));
    if (ret < 0) {
        perror("[k_sendto] sendto() failed");
        return -1;
    }
    printf("[k_sendto] Sent %d bytes on socket index=%d\n", ret, sockfd);
    return ret;
}

int k_recvfrom(int sockfd, void* buf, size_t len, int flags) {
    if (!shm || sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        fprintf(stderr, "[k_recvfrom] Invalid socket\n");
        return ENOTBOUND;
    }

    // Simple approach: If 'has_data' is set, read from that buffer; otherwise, no message
    KTP_Socket* s = &shm->sockets[sockfd];
    if (!s->has_data) {
        // No in-buffer data yet
        return ENOMESSAGE;
    }
    // Copy the data out
    int stored_len = strnlen(s->recv_buffer, MESSAGE_SIZE);
    if ((int)len < stored_len) {
        stored_len = (int)len; // truncate
    }
    memcpy(buf, s->recv_buffer, stored_len);
    s->has_data = 0; // consumed
    memset(s->recv_buffer, 0, MESSAGE_SIZE);

    return stored_len;
}

int k_close(int sockfd) {
    if (!shm || sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        fprintf(stderr, "[k_close] Invalid socket\n");
        return ENOTBOUND;
    }
    KTP_Socket* s = &shm->sockets[sockfd];
    if (s->udp_fd >= 0) {
        close(s->udp_fd);
        s->udp_fd = -1;
    }
    s->is_free = 1;
    printf("[k_close] Socket %d closed\n", sockfd);
    return 0;
}

/*--------------------------------------------------
  THREAD R
  Monitors all non-free sockets. On incoming data:
  - Use recvfrom() and store in socket's recv_buffer
  - Mark has_data=1
--------------------------------------------------*/
void* thread_r(void* arg) {
    ThreadRArgs* tra = (ThreadRArgs*)arg;
    SharedMemory* local_shm = attach_shared_memory(tra->shmid);
    if (!local_shm) {
        fprintf(stderr, "[thread_r] Could not attach to shared memory\n");
        return NULL;
    }
    printf("[thread_r] Thread R started, attached to shared memory\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);

        int max_fd = -1;
        // Collect the active sockets
        for (int i = 0; i < MAX_SOCKETS; i++) {
            if (!local_shm->sockets[i].is_free && local_shm->sockets[i].udp_fd >= 0) {
                FD_SET(local_shm->sockets[i].udp_fd, &readfds);
                if (local_shm->sockets[i].udp_fd > max_fd) {
                    max_fd = local_shm->sockets[i].udp_fd;
                }
            }
        }

        if (max_fd == -1) {
            // No sockets, sleep briefly
            usleep(200000);
            continue;
        }

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int rv = select(max_fd + 1, &readfds, NULL, NULL, &tv);

        if (rv < 0) {
            if (errno == EINTR) {
                // interrupted by signal, just continue
                continue;
            }
            perror("[thread_r] select() returned error");
            usleep(200000);
            continue;
        }
        if (rv == 0) {
            // no activity
            continue;
        }

        // Process ready descriptors
        for (int i = 0; i < MAX_SOCKETS; i++) {
            if (!local_shm->sockets[i].is_free && local_shm->sockets[i].udp_fd >= 0) {
                int fd = local_shm->sockets[i].udp_fd;
                if (FD_ISSET(fd, &readfds)) {
                    char buffer[MESSAGE_SIZE];
                    struct sockaddr_in src;
                    socklen_t srclen = sizeof(src);
                    int ret = recvfrom(fd, buffer, MESSAGE_SIZE, 0, (struct sockaddr*)&src, &srclen);
                    if (ret < 0) {
                        perror("[thread_r] recvfrom() failed");
                        continue;
                    }
                    // Store in the socket's recv_buffer
                    memset(local_shm->sockets[i].recv_buffer, 0, MESSAGE_SIZE);
                    memcpy(local_shm->sockets[i].recv_buffer, buffer, ret > 0 ? ret : 0);
                    local_shm->sockets[i].has_data = 1;
                    printf("[thread_r] Socket %d received %d bytes\n", i, ret);
                }
            }
        }
    }
    return NULL;
}