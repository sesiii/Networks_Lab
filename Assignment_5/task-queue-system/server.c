//Name: Dadi Sasank Kumar
//R NO: 22CS10020
//Assignment 5
//Server code


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
#include <time.h>

#define MAX_BUFFER 1024
#define MAX_TASKS 100
#define PORT 8080
#define MAX_CLIENTS 10
#define IDLE_TIMEOUT 10   
#define TASK_TIMEOUT 15   
#define MAX_TASK_REQUESTS 3  

typedef struct {
    char expression[MAX_BUFFER];
    int assigned;  // 0: not assigned, 1: assigned but not completed, 2: completed
    pid_t client_pid;
    time_t assigned_time;
    double result; // Added to store result
} Task;

typedef struct {
    Task tasks[MAX_TASKS];
    int task_count;
} SharedData;

int shm_id;
SharedData *shared_data;
char task_filename[MAX_BUFFER]; // To store filename for updating

void save_results_to_file() {
    FILE* file = fopen(task_filename, "w");
    if (file == NULL) {
        perror("Error opening task file for writing");
        return;
    }

    for (int i = 0; i < shared_data->task_count; i++) {
        if (shared_data->tasks[i].assigned == 2) {
            fprintf(file, "%s = %.2f\n", shared_data->tasks[i].expression, shared_data->tasks[i].result);
        } else {
            fprintf(file, "%s\n", shared_data->tasks[i].expression);
        }
    }
    fclose(file);
    printf("Results saved to %s\n", task_filename);
}

void handle_sigchld(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Child process %d terminated\n", pid);
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

int load_tasks(const char* filename) {
    strcpy(task_filename, filename);
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening task file");
        return -1;
    }

    char line[MAX_BUFFER];
    shared_data->task_count = 0;

    while (fgets(line, sizeof(line), file) && shared_data->task_count < MAX_TASKS) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            strcpy(shared_data->tasks[shared_data->task_count].expression, line);
            shared_data->tasks[shared_data->task_count].assigned = 0;
            shared_data->tasks[shared_data->task_count].client_pid = -1;
            shared_data->tasks[shared_data->task_count].assigned_time = 0;
            shared_data->tasks[shared_data->task_count].result = 0.0;
            shared_data->task_count++;
        }
    }

    fclose(file);
    printf("Loaded %d tasks\n", shared_data->task_count);
    return 0;
}

int get_next_task() {
    for (int i = 0; i < shared_data->task_count; i++) {
        if (shared_data->tasks[i].assigned == 0) {
            return i;
        }
    }
    return -1;
}

int all_tasks_completed() {
    for (int i = 0; i < shared_data->task_count; i++) {
        if (shared_data->tasks[i].assigned != 2) {
            return 0;
        }
    }
    return 1;
}

void check_task_timeout(int client_socket, pid_t pid) {
    time_t current_time = time(NULL);
    for (int i = 0; i < shared_data->task_count; i++) {
        if (shared_data->tasks[i].assigned == 1 && 
            shared_data->tasks[i].client_pid == pid && 
            shared_data->tasks[i].assigned_time > 0) {
            if (difftime(current_time, shared_data->tasks[i].assigned_time) > TASK_TIMEOUT) {
                printf("Task %s timeout for client %d\n", 
                       shared_data->tasks[i].expression, pid);
                shared_data->tasks[i].assigned = 0;
                shared_data->tasks[i].client_pid = -1;
                shared_data->tasks[i].assigned_time = 0;
                char response[MAX_BUFFER];
                sprintf(response, "Error: Task timeout after %d seconds", TASK_TIMEOUT);
                write(client_socket, response, strlen(response));
                strcpy(response, "exit");
                write(client_socket, response, strlen(response));
                return;
            }
        }
    }
}

void handle_client(int client_socket) {
    char buffer[MAX_BUFFER];
    char response[MAX_BUFFER];
    int bytes_read;
    
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    pid_t pid = getpid();
    printf("Child process %d handling client\n", pid);
    
    time_t last_activity = time(NULL);
    int task_request_count = 0;
    
    while (1) {
        time_t current_time = time(NULL);
        
        if (difftime(current_time, last_activity) > IDLE_TIMEOUT) {
            printf("Client %d idle timeout after %d seconds\n", pid, IDLE_TIMEOUT);
            sprintf(response, "Error: Connection timed out due to inactivity");
            write(client_socket, response, strlen(response));
            // strcpy(response, "exit");
            // write(client_socket, response, strlen(response));
            break;
        }
        
        check_task_timeout(client_socket, pid);
        
        memset(buffer, 0, MAX_BUFFER);
        bytes_read = read(client_socket, buffer, MAX_BUFFER - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Client %d: %s\n", pid, buffer);
            last_activity = time(NULL);
            
            if (strncmp(buffer, "GET_TASK", 8) == 0) {
                int has_task = 0;
                for (int i = 0; i < shared_data->task_count; i++) {
                    if (shared_data->tasks[i].assigned == 1 && shared_data->tasks[i].client_pid == pid) {
                        has_task = 1;
                        task_request_count++;
                        
                        if (task_request_count >= MAX_TASK_REQUESTS) {
                            printf("Client %d exceeded %d task requests without completion\n", 
                                   pid, MAX_TASK_REQUESTS);
                            sprintf(response, "exit");
                            write(client_socket, response, strlen(response));
                            shared_data->tasks[i].assigned = 0;
                            shared_data->tasks[i].client_pid = -1;
                            shared_data->tasks[i].assigned_time = 0;
                            close(client_socket);
                            exit(0);
                        }
                        
                        sprintf(response, "Error: You already have an assigned task. Complete it first. Request count: %d/%d", 
                                task_request_count, MAX_TASK_REQUESTS);
                        write(client_socket, response, strlen(response));
                        break;
                    }
                }
                
                if (!has_task) {
                    task_request_count = 0;
                    int task_index = get_next_task();
                    if (task_index >= 0) {
                        shared_data->tasks[task_index].assigned = 1;
                        shared_data->tasks[task_index].client_pid = pid;
                        shared_data->tasks[task_index].assigned_time = time(NULL);
                        sprintf(response, "Task: %s", shared_data->tasks[task_index].expression);
                        write(client_socket, response, strlen(response));
                        printf("Assigned task %s to client %d\n", 
                               shared_data->tasks[task_index].expression, pid);
                    } else {
                        sprintf(response, "No tasks available, exit");
                        write(client_socket, response, strlen(response));
                        // strcpy(response, "exit");
                        // write(client_socket, response, strlen(response));
                    }
                }
            } else if (strncmp(buffer, "RESULT", 6) == 0) {
                int found_task = 0;
                double result;
                sscanf(buffer, "RESULT %lf", &result);
                
                for (int i = 0; i < shared_data->task_count; i++) {
                    if (shared_data->tasks[i].assigned == 1 && shared_data->tasks[i].client_pid == pid) {
                        printf("Task %s completed by client %d. Result: %.2f\n", 
                               shared_data->tasks[i].expression, pid, result);
                        shared_data->tasks[i].assigned = 2;
                        shared_data->tasks[i].result = result;
                        shared_data->tasks[i].assigned_time = 0;
                        found_task = 1;
                        task_request_count = 0;
                        save_results_to_file();
                        sprintf(response, "Result received. You can request another task.");
                        write(client_socket, response, strlen(response));
                        break;
                    }
                }
                
                if (!found_task) {
                    sprintf(response, "Error: No task was assigned to you.");
                    write(client_socket, response, strlen(response));
                }
                
                if (all_tasks_completed()) {
                    printf("All tasks have been completed!\n");
                    sprintf(response, "No tasks available");
                    write(client_socket, response, strlen(response));
                    strcpy(response, "exit");
                    write(client_socket, response, strlen(response));
                    break;
                }
            } else if (strncmp(buffer, "exit", 4) == 0) {
                printf("Client %d disconnected\n", pid);
                break;
            } else {
                sprintf(response, "Unknown command. Use GET_TASK, RESULT, or exit");
                write(client_socket, response, strlen(response));
            }
        } else if (bytes_read == 0) {
            printf("Client %d disconnected\n", pid);
            break;
        } else if (bytes_read < 0) {
            if (errno != EAGAIN) {
                perror("read error");
                break;
            }
            usleep(10000);
        }
    }
    
    for (int i = 0; i < shared_data->task_count; i++) {
        if (shared_data->tasks[i].assigned == 1 && shared_data->tasks[i].client_pid == pid) {
            printf("Releasing task %s as client %d disconnected\n", 
                   shared_data->tasks[i].expression, pid);
            shared_data->tasks[i].assigned = 0;
            shared_data->tasks[i].client_pid = -1;
            shared_data->tasks[i].assigned_time = 0;
        }
    }
    
    close(client_socket);
    exit(0);
}

void cleanup() {
    save_results_to_file();
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
    
    shm_id = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    
    shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat failed");
        shmctl(shm_id, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    
    memset(shared_data, 0, sizeof(SharedData));
    atexit(cleanup);
    
    if (load_tasks(argv[1]) < 0) {
        exit(EXIT_FAILURE);
    }
    
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_socket, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Task Queue Server started on port %d...\n", PORT);
    
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);
                continue;
            } else {
                perror("accept failed");
                continue;
            }
        }
        
        printf("New client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            close(client_socket);
        } else if (pid == 0) {
            close(server_socket);
            handle_client(client_socket);
            exit(0);
        } else {
            close(client_socket);
        }
    }
    
    close(server_socket);
    return 0;
}