// // #include<stdio.h>
// // #include<stdlib.h>
// // #include<string.h>
// // #include<sys/socket.h>
// // #include<netinet/in.h>
// // #include<arpa/inet.h>
// // #include<unistd.h>

// // #define PORT 8083
// // #define server_ip "127.0.0.1"
// // #define buffer_size 1024
// // int main(int argc, char* argv[]){
    
// //     if(argc!=3){
// //         printf("Usage: %s <server_ip> <port>\n",argv[0]);
// //         exit(1);
// //     }
// //     printf("????client????\n");

// //     int sock_fd;
// //     struct sockaddr_in server_addr;
// //     char buffer[buffer_size];

// //     sock_fd=socket(AF_INET, SOCK_STREAM,0);
// //     if(sock_fd<0){
// //         perror("Socket creation failed");
// //         exit(1);
// //     }

// //     memset(&server_addr,0,sizeof(server_addr));
// //     server_addr.sin_family=AF_INET;
// //     server_addr.sin_port=htons(PORT);
// //     inet_pton(AF_INET,server_ip,&server_addr.sin_addr);

// //     if(connect(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
// //         perror("Connection to server failed.\n");
// //         exit(EXIT_FAILURE);
// //     }

// //     printf("Connected to server.\n");
// //     const char *msg = "Hello, server!";
// //     write(sock_fd, msg, strlen(msg));

// //     int n=read(sock_fd,buffer,buffer_size);
// //     buffer[n]='\0';
// //     printf("Server:%s\n",buffer);

// //     //start process
    
// //     char init_conn[buffer_size];
// //     char client_id[buffer_size];
// //     // printf("\n");
// //     printf("Enter client id: HELO <client_id>\n");
// //     scanf("%s",client_id);

// //     snprintf(init_conn, sizeof(init_conn), "HELO %s", client_id);
// //     write(sock_fd, init_conn, strlen(init_conn));
    
// //     int a=read(sock_fd,buffer,buffer_size);
// //     buffer[a]='\0';
// //     // printf("\n");
// //     printf("Server:%s\n",buffer);

// //     char mail_from[buffer_size];
// //     char sender[buffer_size];
// //     printf("Enter sender email: MAIL FROM:<email>\n");
// //     // printf("\n");
// //     scanf("%s",sender);
// //     snprintf(mail_from, sizeof(mail_from), "MAIL FROM: %s", sender);
// //     write(sock_fd, mail_from, strlen(mail_from));

// //     int b=read(sock_fd,buffer,buffer_size);
// //     buffer[b]='\0';
// //     // printf("\n");
// //     printf("Server:%s\n",buffer);
// //     system("mkdir -p ./inbox");
// //     char rcpt_to[buffer_size];
// //     close(sock_fd);
// //     return 0;

// // }


// // Assignment 6 Submission
// // Name: <Your_Name>
// // Roll number: <Your_Roll_Number>

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <unistd.h>

// #define BUFFER_SIZE 1024

// int main(int argc, char *argv[]) {
//     if (argc != 3) {
//         printf("Usage: %s <server_ip> <port>\n", argv[0]);
//         exit(EXIT_FAILURE);
//     }

//     int sock_fd;
//     struct sockaddr_in server_addr;
//     char buffer[BUFFER_SIZE];
//     int port = atoi(argv[2]);

//     sock_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sock_fd < 0) {
//         perror("Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(port);
//     inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

//     if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         perror("Connection to server failed");
//         exit(EXIT_FAILURE);
//     }

//     printf("Connected to My_SMTP server.\n");

//     while (1) {
//         printf("> ");
//         fgets(buffer, BUFFER_SIZE, stdin);
        
//         write(sock_fd, buffer, strlen(buffer));
        
//         if (strncmp(buffer, "DATA", 4) == 0) {
//             printf("Enter your message (end with a single dot '.'):\n");
//             while (1) {
//                 fgets(buffer, BUFFER_SIZE, stdin);
//                 write(sock_fd, buffer, strlen(buffer));
//                 if (strcmp(buffer, ".\n") == 0) break;
//             }
//         }

//         int n = read(sock_fd, buffer, BUFFER_SIZE - 1);
//         if (n <= 0) {
//             printf("Server disconnected\n");
//             break;
//         }
//         buffer[n] = '\0';
//         printf("%s", buffer);

//         if (strncmp(buffer, "200 Goodbye", 11) == 0) {
//             break;
//         }
//     }

//     close(sock_fd);
//     return 0;
// }


// Assignment 6 Submission
// Name: <Your_Name>
// Roll number: <Your_Roll_Number>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int port = atoi(argv[2]);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to My_SMTP server.\n");

    while (1) {
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        
        write(sock_fd, buffer, strlen(buffer));
        
        if (strncmp(buffer, "DATA", 4) == 0) {
            printf("Enter your message (end with a single dot '.'):\n");
            while (1) {
                fgets(buffer, BUFFER_SIZE, stdin);
                write(sock_fd, buffer, strlen(buffer));
                if (strcmp(buffer, ".\n") == 0) break;
            }
        }

        // Read response
        int total_read = 0;
        do {
            int n = read(sock_fd, buffer + total_read, BUFFER_SIZE - total_read - 1);
            if (n <= 0) {
                printf("Server disconnected\n");
                close(sock_fd);
                return 0;
            }
            total_read += n;
            buffer[total_read] = '\0';
            
            // Check if we've received a complete response
            if (strstr(buffer, "\n")) {
                printf("%s", buffer);
                if (strncmp(buffer, "200 Goodbye", 11) == 0) {
                    close(sock_fd);
                    return 0;
                }
                break;
            }
        } while (total_read < BUFFER_SIZE - 1);
    }

    close(sock_fd);
    return 0;
}