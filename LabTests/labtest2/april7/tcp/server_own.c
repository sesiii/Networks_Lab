#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define COUNT 5

// Shared memory structure
struct shared_data {
    int arr[COUNT];
    int i;
    int ready; // Flag to indicate array is full and sorted
};

void sort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
}

void handle_client(int client_sock, char *client_ip, int client_port, struct shared_data *data) {
    int a_;
    int n = read(client_sock, &a_, sizeof(int));
    if (n <= 0) {
        close(client_sock);
        exit(0);
    }
    printf("Client %s:%d --> %d\n", client_ip, client_port, a_);

    // Critical section: Add number to shared array
    if (data->i < COUNT) {
        data->arr[data->i] = a_;
        data->i++;
        printf("Stored %d at index %d\n", a_, data->i - 1);
    }

    // If array is full, sort and mark as ready
    if (data->i == COUNT && !data->ready) {
        sort(data->arr, COUNT);
        data->ready = 1;
        printf("Array full and sorted\n");
    }

    // If sorted, send the current element back
    if (data->ready) {
        int idx = data->i - COUNT; // Adjust to send first element first
        if (idx >= 0 && idx < COUNT) {
            write(client_sock, &data->arr[idx], sizeof(int));
            printf("Sent %d to %s:%d\n", data->arr[idx], client_ip, client_port);
        }
    }

    close(client_sock);
    exit(0);
}

int main() {
    // Set up shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Shared memory failed");
        exit(1);
    }
    struct shared_data *data = (struct shared_data *)shmat(shmid, NULL, 0);
    if (data == (void *)-1) {
        perror("Attach failed");
        exit(1);
    }
    data->i = 0;
    data->ready = 0;

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    if (listen(sock_fd, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server running on port %d\n", PORT);
    signal(SIGCHLD, SIG_IGN); // Prevent zombie processes

    while (1) {
        int client_sock = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        char client_ip[16];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 16);
        int client_port = ntohs(client_addr.sin_port);
        printf("New client connection (%s:%d)\n", client_ip, client_port);

        pid_t pid = fork();
        if (pid == 0) {
            close(sock_fd);
            handle_client(client_sock, client_ip, client_port, data);
        } else if (pid > 0) {
            close(client_sock);
        } else {
            perror("Fork failed");
        }
    }

    close(sock_fd);
    shmdt(data); // Detach shared memory
    shmctl(shmid, IPC_RMID, NULL); // Clean up shared memory
    return 0;
}