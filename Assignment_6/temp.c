#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#define BUFFER_SIZE 1024
#define MAILBOX_DIR "mailbox/"

void process_command(int client_fd, char *buffer, char *sender, char *recipient, int *in_data, char *email_body);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_fd, max_fd, activity, new_socket, client_socket[30], max_clients = 30, sd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int port = atoi(argv[1]);
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    for (int i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }

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
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_fd) {
                max_fd = sd;
            }
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (new_socket < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

            for (int i = 0; i < max_clients; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                int n = read(sd, buffer, BUFFER_SIZE - 1);
                if (n <= 0) {
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    buffer[n] = '\0';
                    printf("Client: %s", buffer);
                    char sender[256] = {0}, recipient[256] = {0};
                    char email_body[BUFFER_SIZE] = {0};
                    int in_data = 0;
                    process_command(sd, buffer, sender, recipient, &in_data, email_body);
                }
            }
        }
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
        // sprintf(response, "200 OK\n");
        // write(client_fd, response, strlen(response));
        printf("HI");
    }
    else if (*in_data) {
        if (strcmp(buffer, ".\n") == 0) {
            *in_data = 0;
            char filepath[BUFFER_SIZE];
            sprintf(filepath, "%s%s.txt", MAILBOX_DIR, recipient);
            FILE *fp = fopen(filepath, "a");
            if (fp) {
                time(&now);
                tm_info = localtime(&now);
                char date_str[26];
                strftime(date_str, 26, "%d-%m-%Y", tm_info);
                fprintf(fp, "ID: %ld\nFrom: %s\nDate: %s\n%s\n---\n", 
                        time(NULL), sender, date_str, email_body);
                fclose(fp);
                sprintf(response, "200 Message stored successfully\n");
            } else {
                sprintf(response, "500 SERVER ERROR\n");
                printf("HIi am here");
            }
            write(client_fd, response, strlen(response));
        } else {
            strncat(email_body, buffer, BUFFER_SIZE - strlen(email_body) - 1);
        }
    }
    else if (strncmp(buffer, "LIST", 4) == 0) {
        char list_recipient[256];
        if (sscanf(buffer, "LIST %s", list_recipient) != 1) {
            sprintf(response, "400 ERR\n");
            write(client_fd, response, strlen(response));
            return;
        }
        printf("LIST %s\n", list_recipient);
        
        char filepath[BUFFER_SIZE];
        sprintf(filepath, "%s%s.txt", MAILBOX_DIR, list_recipient);
        FILE *fp = fopen(filepath, "r");
        
        sprintf(response, "200 OK\n");
        write(client_fd, response, strlen(response));
        
        if (fp) {
            char line[BUFFER_SIZE];
            int email_count = 0;
            char from[256] = {0};
            char date[26] = {0};
            
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "From:", 5) == 0) {
                    sscanf(line, "From: %s", from);
                }
                else if (strncmp(line, "Date:", 5) == 0) {
                    sscanf(line, "Date: %s", date);
                    email_count++;
                    sprintf(response, "%d: Email from %s (%s)\n", 
                            email_count, from, date);
                    write(client_fd, response, strlen(response));
                }
            }
            fclose(fp);
            if (email_count == 0) {
                sprintf(response, "No emails found\n");
                write(client_fd, response, strlen(response));
            }
            printf("Emails retrieved; list sent.\n");
        } else {
            sprintf(response, "No emails found\n");
            write(client_fd, response, strlen(response));
        }
        // Add this line to ensure the end of the list is properly handled
        sprintf(response, "End of list\n");
        write(client_fd, response, strlen(response));
    }
    else if (strncmp(buffer, "GET_MAIL", 8) == 0) {
        char get_recipient[256];
        int email_id;
        if (sscanf(buffer, "GET_MAIL %s %d", get_recipient, &email_id) != 2) {
            sprintf(response, "400 ERR\n");
            write(client_fd, response, strlen(response));
            return;
        }
        
        char filepath[BUFFER_SIZE];
        sprintf(filepath, "%s%s.txt", MAILBOX_DIR, get_recipient);
        FILE *fp = fopen(filepath, "r");
        if (!fp) {
            sprintf(response, "401 NOT FOUND\n");
            write(client_fd, response, strlen(response));
            return;
        }

        char line[BUFFER_SIZE];
        int current_id = 0;
        char email_content[BUFFER_SIZE] = {0};
        int in_email = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "ID:", 3) == 0) {
                current_id++;
                if (current_id == email_id) {
                    in_email = 1;
                } else {
                    in_email = 0;
                }
                memset(email_content, 0, BUFFER_SIZE);
            }
            if (in_email) {
                strncat(email_content, line, BUFFER_SIZE - strlen(email_content) - 1);
                if (strncmp(line, "---", 3) == 0) {
                    break;
                }
            }
        }
        fclose(fp);
        
        if (strlen(email_content) > 0) {
            sprintf(response, "200 OK\n%s", email_content);
            write(client_fd, response, strlen(response));
        } else {
            sprintf(response, "401 NOT FOUND\n");
            write(client_fd, response, strlen(response));
        }
    }
    else if (strcmp(buffer, "QUIT\n") == 0) {
        sprintf(response, "200 Goodbye\n");
        write(client_fd, response, strlen(response));
        close(client_fd);
    }
    else {
        sprintf(response, "400 ERR\n");
        write(client_fd, response, strlen(response));
    }
}