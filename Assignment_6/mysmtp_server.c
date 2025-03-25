

// Assignment 6 Submission
// Name: Dadi Sasank Kumar
// Roll number: 22CS10020

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
#define MAILBOX_DIR "mailbox/"

void handle_client(int client_fd, struct sockaddr_in client_addr);
void process_command(int client_fd, char *buffer, char *sender, char *recipient, int *in_data, char *email_body, int *state);
sem_t *get_recipient_semaphore(const char *recipient);


enum
{
    STATE_INIT = 0,
    STATE_HELO = 1,
    STATE_MAIL_FROM = 2,
    STATE_RCPT_TO = 3,
    STATE_DATA = 4
};

int server_fd;



int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int port = atoi(argv[1]);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", port);
    fflush(stdout);

    mkdir(MAILBOX_DIR, 0777);

    while (1)
    {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("Accept failed");
            continue;
        }
        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid == 0)
        {
            close(server_fd);
            handle_client(client_fd, client_addr);
            close(client_fd);
            exit(0);
        }
        else if (pid > 0)
        {
            close(client_fd);
            while (waitpid(-1, NULL, WNOHANG) > 0)
                ;
        }
        else
        {
            perror("Fork failed");
            close(client_fd);
        }
    }
    close(server_fd);
    return 0;
}

void handle_client(int client_fd, struct sockaddr_in client_addr)
{
    char buffer[BUFFER_SIZE];
    char sender[256] = {0}, recipient[256] = {0};
    char email_body[BUFFER_SIZE] = {0};
    int in_data = 0;
    int state = STATE_INIT;

    while (1)
    {
        int n = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (n <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }
        buffer[n] = '\0';
        if (buffer[n - 1] != '\n')
            continue;
        process_command(client_fd, buffer, sender, recipient, &in_data, email_body, &state);
    }
}

void process_command(int client_fd, char *buffer, char *sender, char *recipient, int *in_data, char *email_body, int *state)
{
    char response[BUFFER_SIZE];
    time_t now;
    struct tm *tm_info;

    if (strlen(buffer) <= 1)
    {
        sprintf(response, "400 ERR\n");
        write(client_fd, response, strlen(response));
        return;
    }

    if (strncmp(buffer, "HELO", 4) == 0)
    {
        char client_id[256];
        if (sscanf(buffer, "HELO %255s", client_id) != 1)
        {
            sprintf(response, "400 ERR\n");
            write(client_fd, response, strlen(response));
        }
        else
        {
            *state = STATE_HELO;
            printf("HELO received from %s\n", client_id);
            sprintf(response, "200 OK\n");
            write(client_fd, response, strlen(response));
        }
    }
    else if (strncmp(buffer, "MAIL FROM:", 10) == 0)
    {
        if (*state < STATE_HELO)
        {
            sprintf(response, "403 FORBIDDEN\n");
            write(client_fd, response, strlen(response));
            *state = STATE_INIT;
            memset(sender, 0, 256);
            memset(recipient, 0, 256);
            memset(email_body, 0, BUFFER_SIZE);
            *in_data = 0;
            return;
        }
        if (sscanf(buffer, "MAIL FROM: %255s", sender) != 1 || strchr(sender, '@') == NULL)
        {
            sprintf(response, "400 ERR\n");
            write(client_fd, response, strlen(response));
        }
        else
        {
            *state = STATE_MAIL_FROM;
            printf("MAIL FROM: %s\n", sender);
            sprintf(response, "200 OK\n");
            write(client_fd, response, strlen(response));
        }
    }
    else if (strncmp(buffer, "RCPT TO:", 8) == 0)
    {
        if (*state < STATE_MAIL_FROM)
        {
            sprintf(response, "403 FORBIDDEN\n");
            write(client_fd, response, strlen(response));
            *state = STATE_INIT;
            memset(sender, 0, 256);
            memset(recipient, 0, 256);
            memset(email_body, 0, BUFFER_SIZE);
            *in_data = 0;
            return;
        }
        if (sscanf(buffer, "RCPT TO: %255s", recipient) != 1 || strchr(recipient, '@') == NULL)
        {
            sprintf(response, "400 ERR\n");
            write(client_fd, response, strlen(response));
        }
        else
        {
            *state = STATE_RCPT_TO;
            printf("RCPT TO: %s\n", recipient);
            sprintf(response, "200 OK\n");
            write(client_fd, response, strlen(response));
        }
    }
    else if (strcmp(buffer, "DATA\n") == 0)
    {
        if (*state < STATE_RCPT_TO)
        {
            sprintf(response, "403 FORBIDDEN\n");
            write(client_fd, response, strlen(response));
            *state = STATE_INIT;
            memset(sender, 0, 256);
            memset(recipient, 0, 256);
            memset(email_body, 0, BUFFER_SIZE);
            *in_data = 0;
            return;
        }
        *in_data = 1;
        *state = STATE_DATA;
        memset(email_body, 0, BUFFER_SIZE);
        sprintf(response, "Enter your message (end with a single dot '.'):\n");
        write(client_fd, response, strlen(response));
    }
    else if (*in_data)
    {
        if (strcmp(buffer, ".\n") == 0)
        {
            *in_data = 0;
            sem_t *sem = get_recipient_semaphore(recipient);
            sem_wait(sem);
            char filepath[BUFFER_SIZE];
            sprintf(filepath, "%s%s.txt", MAILBOX_DIR, recipient);
            FILE *fp = fopen(filepath, "a");
            if (fp)
            {
                time(&now);
                tm_info = localtime(&now);
                char date_str[26];
                strftime(date_str, 26, "%d-%m-%Y", tm_info);
                fprintf(fp, "From: %s\nDate: %s\n%s\n---\n",
                         sender, date_str, email_body);
                fclose(fp);
                printf("DATA received, message stored.\n");
                sprintf(response, "250 Message stored successfully\n");
            }
            else
            {
                sprintf(response, "500 SERVER ERROR\n");
            }
            sem_post(sem);
            write(client_fd, response, strlen(response));
            *state = STATE_HELO; 

        }
        else
        {
            printf("%s", buffer);
            strncat(email_body, buffer, BUFFER_SIZE - strlen(email_body) - 1);
        }
    }
    else if (strncmp(buffer, "LIST", 4) == 0)
    {
        char list_recipient[256];
        if (sscanf(buffer, "LIST %255s", list_recipient) != 1)
        {
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

        if (fp)
        {
            char line[BUFFER_SIZE];
            int email_count = 0;
            char from[256] = {0};
            char date[26] = {0};

            while (fgets(line, sizeof(line), fp))
            {
                if (strncmp(line, "From:", 5) == 0)
                {
                    sscanf(line, "From: %255s", from);
                }
                else if (strncmp(line, "Date:", 5) == 0)
                {
                    sscanf(line, "Date: %25s", date);
                    email_count++;
                    sprintf(response, "%d: Email from %s (%s)\n",
                            email_count, from, date);
                    write(client_fd, response, strlen(response));
                }
            }
            fclose(fp);
            if (email_count == 0)
            {
                sprintf(response, "No emails found\n");
                write(client_fd, response, strlen(response));
            }
            sprintf(response, "---\n");
            write(client_fd, response, strlen(response));
            printf("Emails retrieved; list sent.\n");
        }
        else
        {
            sprintf(response, "No emails found\n---\n");
            write(client_fd, response, strlen(response));
        }
    }
    else if (strncmp(buffer, "GET_MAIL", 8) == 0)
    {
        char get_recipient[256];
        int email_id;
        if (sscanf(buffer, "GET_MAIL %255s %d", get_recipient, &email_id) != 2)
        {
            sprintf(response, "400 ERR\n");
            write(client_fd, response, strlen(response));
            return;
        }
        printf("GET_MAIL %s %d\n", get_recipient, email_id);

        char filepath[BUFFER_SIZE];
        sprintf(filepath, "%s%s.txt", MAILBOX_DIR, get_recipient);
        FILE *fp = fopen(filepath, "r");
        if (!fp)
        {
            sprintf(response, "401 NOT FOUND\n");
            write(client_fd, response, strlen(response));
            return;
        }

        char line[BUFFER_SIZE];
        int current_id = 0;
        char email_content[BUFFER_SIZE] = {0};
        int in_email = 0;

        while (fgets(line, sizeof(line), fp))
        {
            if (strncmp(line, "ID:", 3) == 0)
            {
                current_id++;
                if (current_id == email_id)
                {
                    in_email = 1;
                }
                else
                {
                    in_email = 0;
                    memset(email_content, 0, BUFFER_SIZE);
                }
            }
            if (in_email)
            {
                strncat(email_content, line, BUFFER_SIZE - strlen(email_content) - 1);
                if (strncmp(line, "---", 3) == 0)
                {
                    break;
                }
            }
        }
        fclose(fp);

        if (in_email && strlen(email_content) > 0)
        {
            sprintf(response, "200 OK\n%s---\n", email_content);
            write(client_fd, response, strlen(response));
            printf("Email with id %d sent.\n", email_id);
        }
        else
        {
            sprintf(response, "401 NOT FOUND\n");
            write(client_fd, response, strlen(response));
        }
    }
    else if (strcmp(buffer, "QUIT\n") == 0)
    {
        printf("Client disconnected.\n");
        sprintf(response, "200 Goodbye\n");
        write(client_fd, response, strlen(response));
        close(client_fd);
        exit(0);
    }
    else
    {
        sprintf(response, "400 ERR\n");
        write(client_fd, response, strlen(response));
    }
}

sem_t *get_recipient_semaphore(const char *recipient)
{
    char sem_name[256];
    sprintf(sem_name, "/sem_%s", recipient);
    sem_t *sem = sem_open(sem_name, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED)
    {
        perror("Semaphore creation failed");
        exit(EXIT_FAILURE);
    }
    return sem;
}