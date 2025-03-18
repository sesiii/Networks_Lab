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
// // int main(){
    
// //     int client_fd,server_fd;
// //     struct sockaddr_in server_addr,client_addr;
// //     char buffer[buffer_size];
// //     socklen_t client_len=sizeof(client_addr);

// //     server_fd=socket(AF_INET, SOCK_STREAM,0);
// //     if(server_fd<0){
// //         perror("Socket creation failed");
// //         exit(EXIT_FAILURE);
// //     }

// //     printf("????server????\n");

// //     memset(&server_addr,0,sizeof(server_addr));
// //     server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
// //     server_addr.sin_family=AF_INET;
// //     server_addr.sin_port=htons(PORT);

// //     // printf("ih");
// //     if(bind(server_fd,(const struct sockaddr*)&server_addr,sizeof(server_addr))<0){
// //         perror("Binding failed");
// //         exit(EXIT_FAILURE);
// //     }

// //     if(listen(server_fd,5)<0){
// //         perror("Listen failed.\n");
// //         exit(EXIT_FAILURE);
// //     }

// //     printf("TCP server listening on port: %d\n",PORT);

// //     client_fd=accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
// //     if(client_fd<0){
// //         perror("Accept failed.\n");
// //         exit(EXIT_FAILURE);
// //     }

// //     printf("Client connected.\n");

// //     int n=read(client_fd,buffer,buffer_size);
// //     buffer[n]='\0';
// //     printf("Client:%s",buffer);
// //     printf("\n");
// //     const char *msg = "Hello, client!";
// //     write(client_fd, msg, strlen(msg));

// //     //start process
// //     //HELLO
// //     int m=read(client_fd,buffer,buffer_size);
// //     buffer[m]='\0';
// //     printf("Client:%s\n",buffer);

// //     char *a="Connection established.";
// //     write(client_fd, a, strlen(a));

// //     //MAIL FROM
// //     int b=read(client_fd,buffer,buffer_size);
// //     buffer[b]='\0';
// //     printf("Client:%s\n",buffer);

// //     char *c="Sender email received.";
// //     write(client_fd, c, strlen(c));

// //     close(client_fd);

// //     close(server_fd);
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
// #include <sys/stat.h>

// #define BUFFER_SIZE 1024
// #define MAILBOX_DIR "mailbox/"

// void process_command(int client_fd, char *buffer, char *sender, char *recipient, int *in_data);

// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         printf("Usage: %s <port>\n", argv[0]);
//         exit(EXIT_FAILURE);
//     }

//     int server_fd, client_fd;
//     struct sockaddr_in server_addr, client_addr;
//     char buffer[BUFFER_SIZE];
//     socklen_t client_len = sizeof(client_addr);
//     int port = atoi(argv[1]);

//     server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd < 0) {
//         perror("Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(port);

//     if (bind(server_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         perror("Binding failed");
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 5) < 0) {
//         perror("Listen failed");
//         exit(EXIT_FAILURE);
//     }

//     printf("Listening on port %d...\n", port);
//     mkdir(MAILBOX_DIR, 0777);

//     while (1) {
//         client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
//         if (client_fd < 0) {
//             perror("Accept failed");
//             continue;
//         }

//         printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
        
//         char sender[256] = {0}, recipient[256] = {0};
//         int in_data = 0;

//         while (1) {
//             int n = read(client_fd, buffer, BUFFER_SIZE - 1);
//             if (n <= 0) {
//                 printf("Client disconnected\n");
//                 break;
//             }
//             buffer[n] = '\0';
//             printf("Client: %s", buffer);
//             process_command(client_fd, buffer, sender, recipient, &in_data);
//         }
//         close(client_fd);
//     }

//     close(server_fd);
//     return 0;
// }

// void process_command(int client_fd, char *buffer, char *sender, char *recipient, int *in_data) {
//     char response[BUFFER_SIZE];
//     char email_body[BUFFER_SIZE] = {0};

//     if (strncmp(buffer, "HELO", 4) == 0) {
//         sprintf(response, "200 OK\n");
//         write(client_fd, response, strlen(response));
//     }
//     else if (strncmp(buffer, "MAIL FROM:", 10) == 0) {
//         sscanf(buffer, "MAIL FROM: %s", sender);
//         sprintf(response, "200 OK\n");
//         write(client_fd, response, strlen(response));
//     }
//     else if (strncmp(buffer, "RCPT TO:", 8) == 0) {
//         sscanf(buffer, "RCPT TO: %s", recipient);
//         sprintf(response, "200 OK\n");
//         write(client_fd, response, strlen(response));
//     }
//     else if (strcmp(buffer, "DATA\n") == 0) {
//         *in_data = 1;
//         sprintf(response, "200 OK\n");
//         write(client_fd, response, strlen(response));
//     }
//     else if (*in_data) {
//         if (strcmp(buffer, ".\n") == 0) {
//             *in_data = 0;
//             // Store email
//             char filepath[BUFFER_SIZE];
//             sprintf(filepath, "%s/%s.txt", MAILBOX_DIR, recipient);
//             FILE *fp = fopen(filepath, "a");
//             if (fp) {
//                 fprintf(fp, "From: %s\n%s\n---\n", sender, email_body);
//                 fclose(fp);
//             }
//             sprintf(response, "200 Message stored successfully\n");
//             write(client_fd, response, strlen(response));
//         } else {
//             strcat(email_body, buffer);
//         }
//     }
//     else if (strncmp(buffer, "QUIT", 4) == 0) {
//         sprintf(response, "200 Goodbye\n");
//         write(client_fd, response, strlen(response));
//         close(client_fd);
//     }
//     else {
//         sprintf(response, "400 ERR\n");
//         write(client_fd, response, strlen(response));
//     }
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
#include <sys/stat.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAILBOX_DIR "mailbox/"

void process_command(int client_fd, char *buffer, char *sender, char *recipient, int *in_data, char *email_body);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr);
    int port = atoi(argv[1]);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", port);
    mkdir(MAILBOX_DIR, 0777);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
        
        char sender[256] = {0}, recipient[256] = {0};
        char email_body[BUFFER_SIZE] = {0};
        int in_data = 0;

        while (1) {
            int n = read(client_fd, buffer, BUFFER_SIZE - 1);
            if (n <= 0) {
                printf("Client disconnected\n");
                break;
            }
            buffer[n] = '\0';
            printf("Client: %s", buffer);
            process_command(client_fd, buffer, sender, recipient, &in_data, email_body);
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

void process_command(int client_fd, char *buffer, char *sender, char *recipient, int *in_data, char *email_body) {
    char response[BUFFER_SIZE];
    time_t now;
    struct tm *tm_info;

    if (strncmp(buffer, "HELO", 4) == 0) {
        sprintf(response, "200 OK\n");
        write(client_fd, response, strlen(response));
    }
    else if (strncmp(buffer, "MAIL FROM:", 10) == 0) {
        sscanf(buffer, "MAIL FROM: %s", sender);
        sprintf(response, "200 OK\n");
        write(client_fd, response, strlen(response));
    }
    else if (strncmp(buffer, "RCPT TO:", 8) == 0) {
        sscanf(buffer, "RCPT TO: %s", recipient);
        sprintf(response, "200 OK\n");
        write(client_fd, response, strlen(response));
    }
    else if (strcmp(buffer, "DATA\n") == 0) {
        *in_data = 1;
        memset(email_body, 0, BUFFER_SIZE);
        sprintf(response, "200 OK\n");
        write(client_fd, response, strlen(response));
    }
    else if (*in_data) {
        if (strcmp(buffer, ".\n") == 0) {
            *in_data = 0;
            // Store email
            char filepath[BUFFER_SIZE];
            sprintf(filepath, "%s%s.txt", MAILBOX_DIR, recipient);
            FILE *fp = fopen(filepath, "a");
            if (fp) {
                time(&now);
                tm_info = localtime(&now);
                char date_str[26];
                strftime(date_str, 26, "%d-%m-%Y %H:%M:%S", tm_info);
                fprintf(fp, "ID: %ld\nFrom: %s\nDate: %s\n%s\n---\n", 
                       time(NULL), sender, date_str, email_body);
                fclose(fp);
                sprintf(response, "200 Message stored successfully\n");
            } else {
                sprintf(response, "500 SERVER ERROR\n");
            }
            write(client_fd, response, strlen(response));
        } else {
            strncat(email_body, buffer, BUFFER_SIZE - strlen(email_body) - 1);
        }
    }
    else if (strncmp(buffer, "LIST", 4) == 0) {
        char list_recipient[256];
        sscanf(buffer, "LIST %s", list_recipient);
        char filepath[BUFFER_SIZE];
        sprintf(filepath, "%s%s.txt", MAILBOX_DIR, list_recipient);
        FILE *fp = fopen(filepath, "r");
        if (fp) {
            sprintf(response, "200 OK\n");
            write(client_fd, response, strlen(response));
            
            char line[BUFFER_SIZE];
            int email_count = 0;
            char from[256];
            char date[26];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "From:", 5) == 0) {
                    sscanf(line, "From: %s", from);
                }
                if (strncmp(line, "Date:", 5) == 0) {
                    sscanf(line, "Date: %s", date);
                    email_count++;
                    sprintf(response, "%d: Email from %s (%s)\n", 
                           email_count, from, date);
                    write(client_fd, response, strlen(response));
                }
            }
            fclose(fp);
        } else {
            sprintf(response, "401 NOT FOUND\n");
            write(client_fd, response, strlen(response));
        }
    }
    else if (strncmp(buffer, "QUIT", 4) == 0) {
        sprintf(response, "200 Goodbye\n");
        write(client_fd, response, strlen(response));
        close(client_fd);
    }
    else {
        sprintf(response, "400 ERR\n");
        write(client_fd, response, strlen(response));
    }
}