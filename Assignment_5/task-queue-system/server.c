/*
=====================================
Assignment 5 Submission
Name: Your_Name
Roll number: Your_Roll_Number
=====================================
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_BUFFER 1024
#define MAX_TASKS 100
#define PORT 8080
#define MAX_CLIENTS 10

// Structure to represent a task
typedef struct {
    char expression[MAX_BUFFER];
    int assigned;  // 0: not assigned, 1: assigned but not completed, 2: completed
    pid_t client_pid; // To track which client is handling this task
} Task;

// Shared memory structure
typedef struct {
    Task tasks[MAX_TASKS];
    int task_count;
} SharedData;

// Global variables
int shm_id;
SharedData *shared_data;

// Function to handle child process termination
void handle_sigchld(int sig) {
    // Non-blocking wait to prevent zombie processes
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Child process %d terminated\n", pid);
        
        // Release any task assigned to this process
        for (int i = 0; i < shared_data->task_count; i++) {
            if (shared_data->tasks[i].assigned == 1 && shared_data->tasks[i].client_pid == pid) {
                printf("Releasing task %s as client with PID %d disconnected\n", 
                       shared_data->tasks[i].expression, pid);
                shared_data->tasks[i].assigned = 0;
                shared_data->tasks[i].client_pid = -1;
            }
        }
    }
}

// Function to load tasks from config file
int load_tasks(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening task file");
        return -1;
    }

    char line[MAX_BUFFER];
    shared_data->task_count = 0;

    while (fgets(line, sizeof(line), file) && shared_data->task_count < MAX_TASKS) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) > 0) {
            strcpy(shared_data->tasks[shared_data->task_count].expression, line);
            shared_data->tasks[shared_data->task_count].assigned = 0;
            shared_data->tasks[shared_data->task_count].client_pid = -1;
            shared_data->task_count++;
        }
    }

    fclose(file);
    printf("Loaded %d tasks\n", shared_data->task_count);
    return 0;
}

// Function to get next available task
int get_next_task() {
    for (int i = 0; i < shared_data->task_count; i++) {
        if (shared_data->tasks[i].assigned == 0) {
            return i;
        }
    }
    return -1;  // No tasks available
}

// Function to check if all tasks are completed
int all_tasks_completed() {
    for (int i = 0; i < shared_data->task_count; i++) {
        if (shared_data->tasks[i].assigned != 2) {
            return 0;  // Not all tasks are completed
        }
    }
    return 1;  // All tasks are completed
}

// Function to handle a client connection
void handle_client(int client_socket) {
    char buffer[MAX_BUFFER];
    char response[MAX_BUFFER];
    int bytes_read;
    
    // Set socket to non-blocking mode
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    pid_t pid = getpid();
    printf("Child process %d handling client\n", pid);

    while (1) {
        // Clear the buffer
        memset(buffer, 0, MAX_BUFFER);
        
        // Non-blocking read
        bytes_read = read(client_socket, buffer, MAX_BUFFER - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Client %d: %s\n", pid, buffer);
            
            // Process client commands
            if (strncmp(buffer, "GET_TASK", 8) == 0) {
                // Check if client already has an assigned task
                int has_task = 0;
                for (int i = 0; i < shared_data->task_count; i++) {
                    if (shared_data->tasks[i].assigned == 1 && shared_data->tasks[i].client_pid == pid) {
                        has_task = 1;
                        sprintf(response, "Error: You already have an assigned task. Complete it first.");
                        write(client_socket, response, strlen(response));
                        break;
                    }
                }
                
                if (!has_task) {
                    // Get next available task
                    int task_index = get_next_task();
                    if (task_index >= 0) {
                        // Mark task as assigned
                        shared_data->tasks[task_index].assigned = 1;
                        shared_data->tasks[task_index].client_pid = pid;
                        
                        // Send task to client
                        sprintf(response, "Task: %s", shared_data->tasks[task_index].expression);
                        write(client_socket, response, strlen(response));
                        
                        printf("Assigned task %s to client %d\n", 
                               shared_data->tasks[task_index].expression, pid);
                    } else {
                        // No tasks available
                        sprintf(response, "No tasks available");
                        write(client_socket, response, strlen(response));
                    }
                }
            } else if (strncmp(buffer, "RESULT", 6) == 0) {
                // Process result
                int found_task = 0;
                
                for (int i = 0; i < shared_data->task_count; i++) {
                    if (shared_data->tasks[i].assigned == 1 && shared_data->tasks[i].client_pid == pid) {
                        printf("Task %s completed by client %d. Result: %s\n", 
                               shared_data->tasks[i].expression, pid, buffer + 7);
                        shared_data->tasks[i].assigned = 2;  // Mark as completed
                        found_task = 1;
                        
                        sprintf(response, "Result received. You can request another task.");
                        write(client_socket, response, strlen(response));
                        break;
                    }
                }
                
                if (!found_task) {
                    sprintf(response, "Error: No task was assigned to you.");
                    write(client_socket, response, strlen(response));
                }
                
                // Check if all tasks are completed
                if (all_tasks_completed()) {
                    printf("All tasks have been completed!\n");
                    // break;
                    close(client_socket);
                    exit(0);
                }
            } else if (strncmp(buffer, "exit", 4) == 0) {
                printf("Client %d disconnected\n", pid);
                break;
            } else {
                // Unknown command
                sprintf(response, "Unknown command. Use GET_TASK, RESULT, or exit");
                write(client_socket, response, strlen(response));
            }
        } else if (bytes_read == 0) {
            // Client disconnected
            printf("Client %d disconnected\n", pid);
            break;
        } else if (bytes_read < 0) {
            // EAGAIN means no data available in non-blocking mode
            if (errno != EAGAIN) {
                perror("read error");
                break;
            }
            // Sleep a bit to prevent CPU hogging
            usleep(10000);  // 10ms
        }
    }
    
    // Release any assigned task before exiting
    for (int i = 0; i < shared_data->task_count; i++) {
        if (shared_data->tasks[i].assigned == 1 && shared_data->tasks[i].client_pid == pid) {
            printf("Releasing task %s as client %d disconnected\n", 
                   shared_data->tasks[i].expression, pid);
            shared_data->tasks[i].assigned = 0;
            shared_data->tasks[i].client_pid = -1;
        }
    }
    
    close(client_socket);
    exit(0);
}

// Function to clean up resources before exit
void cleanup() {
    // Detach and remove shared memory
    if (shared_data) {
        shmdt(shared_data);
    }
    if (shm_id >= 0) {
        shmctl(shm_id, IPC_RMID, NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <task_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Set up shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    
    // Attach shared memory
    shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat failed");
        shmctl(shm_id, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    
    // Initialize shared data
    memset(shared_data, 0, sizeof(SharedData));
    
    // Register cleanup function
    atexit(cleanup);
    
    // Load tasks from file
    if (load_tasks(argv[1]) < 0) {
        exit(EXIT_FAILURE);
    }
    
    // Set up SIGCHLD handler to prevent zombie processes
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Task Queue Server started on port %d...\n", PORT);
    
    // Set server socket to non-blocking mode
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
    
    // Main server loop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Non-blocking accept
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connections available, not an error in non-blocking mode
                usleep(10000);  // Sleep 10ms to prevent CPU hogging
                continue;
            } else {
                perror("accept failed");
                continue;
            }
        }
        
        // New client connected
        printf("New client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Fork a child process to handle the client
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            close(client_socket);
        } else if (pid == 0) {
            // Child process
            close(server_socket);  // Close server socket in child
            
            // Handle client connection
            handle_client(client_socket);
            exit(0);
        } else {
            // Parent process
            close(client_socket);  // Close client socket in parent
        }
        printf("Completed all the tasks.\n");
    }
    
    close(server_socket);
    // Close server socket
    
    return 0;
}