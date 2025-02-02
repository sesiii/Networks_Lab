/*
 =====================================
 Assignment 3 Submission
 Name: Dadi Sasank Kumar
 Roll number: 22CS10020
 Link of the pcap file: https://drive.google.com/file/d/1YQgClovYTnGYSRxy4LBnBTZV7oCTf11U/view?usp=sharing
 =====================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 10111
#define BUFFER_SIZE 50
#define SERVER_CHUNK_SIZE 3 // Server's chunk size for sending
#define MAX_PENDING 4  // Maximum number of pending connections

// Function to get current timestamp
void get_timestamp(char *buffer) {
    time_t now = time(NULL);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", gmtime(&now));
}

// Function to encrypt a character using the substitution cipher
char encrypt_char(char c, const char* key) {
    if ((c >= 'A' && c <= 'Z')) {
        return key[c - 'A'];
    } else if (c >= 'a' && c <= 'z') {
        return key[c - 'a'] + ('a' - 'A');
    }
    return c;
}

// Function to encrypt the file contents
void encrypt_file(const char* input_file, const char* output_file, const char* key) {
    FILE *fin = fopen(input_file, "r");
    FILE *fout = fopen(output_file, "w");
    char c;
    
    while ((c = fgetc(fin)) != EOF) {
        fputc(encrypt_char(c, key), fout);
    }
    
    fclose(fin);
    fclose(fout);
}

int main() {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    int clilen;
    char timestamp[26];
    
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Cannot create socket");
        exit(0);
    }
    
    // Set up server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    
    // Bind socket to address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Unable to bind local address");
        exit(0);
    }
    
    // Listen for connections
    listen(sockfd, MAX_PENDING);
    
    printf("Server started on port %d\n", PORT);
    
    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        
        if (newsockfd < 0) {
            perror("Accept error");
            continue;
        }
        
        // Get client information
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(cli_addr.sin_port);
        
        get_timestamp(timestamp);
        printf("[%s] New connection from %s:%d\n", timestamp, client_ip, client_port);
        
        // Fork a child process to handle this client
        if (fork() == 0) {
            // Child process
            close(sockfd);  // Close original socket in child
            
            char recv_buffer[BUFFER_SIZE] = {0};
            char send_buffer[SERVER_CHUNK_SIZE] = {0};
            char key[27] = {0};
            
            // Receive encryption key
            int key_bytes = recv(newsockfd, key, 26, 0);
            if (key_bytes <= 0) {
                close(newsockfd);
                exit(0);
            }
            key[26] = '\0';
            
            // Create temporary file name
            char temp_filename[100];
            snprintf(temp_filename, sizeof(temp_filename), "%s.%d.txt", client_ip, client_port);
            
            get_timestamp(timestamp);
            printf("[%s] Processing file for %s:%d\n", timestamp, client_ip, client_port);
            
            // Create encrypted file name
            char enc_filename[100];
            snprintf(enc_filename, sizeof(enc_filename), "%s.enc", temp_filename);
            
            // Open temporary file for writing
            FILE *temp_file = fopen(temp_filename, "w");
            if (!temp_file) {
                perror("Failed to create temporary file");
                close(newsockfd);
                exit(0);
            }
            
            // Receive file content in chunks
            while (1) {
                memset(recv_buffer, 0, BUFFER_SIZE);
                int bytes_received = recv(newsockfd, recv_buffer, BUFFER_SIZE - 1, 0);
                if (bytes_received <= 0) break;
                
                // Check for end-of-file marker
                if (bytes_received == 1 && recv_buffer[0] == '\0') break;
                
                size_t bytes_written = 0;
                while (bytes_written < bytes_received) {
                    size_t written = fwrite(recv_buffer + bytes_written, 1, 
                                         bytes_received - bytes_written, temp_file);
                    if (written <= 0) {
                        perror("Error writing to temporary file");
                        fclose(temp_file);
                        close(newsockfd);
                        exit(0);
                    }
                    bytes_written += written;
                }
            }
            fclose(temp_file);
            
            // Encrypt the file
            encrypt_file(temp_filename, enc_filename, key);
            
            // Send encrypted file back in chunks
            FILE *enc_file = fopen(enc_filename, "r");
            if (enc_file) {
                while (1) {
                    memset(send_buffer, 0, SERVER_CHUNK_SIZE);
                    size_t bytes_read = fread(send_buffer, 1, SERVER_CHUNK_SIZE - 1, enc_file);
                    if (bytes_read <= 0) break;
                    
                    size_t bytes_sent = 0;
                    while (bytes_sent < bytes_read) {
                        int sent = send(newsockfd, send_buffer + bytes_sent, 
                                     bytes_read - bytes_sent, 0);
                        if (sent <= 0) {
                            perror("Error sending encrypted chunk");
                            fclose(enc_file);
                            close(newsockfd);
                            exit(0);
                        }
                        bytes_sent += sent;
                    }
                    
                    usleep(5000);  // 5ms delay between chunks
                }
                fclose(enc_file);
            }
            
            // Clean up temporary files
            remove(temp_filename);
            remove(enc_filename);
            
            get_timestamp(timestamp);
            printf("[%s] Completed processing for %s:%d\n", timestamp, client_ip, client_port);
            
            close(newsockfd);
            exit(0);
        }
        
        // Parent process
        close(newsockfd);
    }
    
    return 0;
}