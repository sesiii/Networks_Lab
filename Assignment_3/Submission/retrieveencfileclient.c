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
#include <arpa/inet.h>
#include <time.h>

#define PORT 10111
#define BUFFER_SIZE 50
#define CHUNK_SIZE 5  // Client's chunk size for sending

// Function to verify the encryption key
int verify_key(const char* key) {
    if (strlen(key) != 26) return 0;
    return 1;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char chunk[CHUNK_SIZE] = {0};
    char recv_buffer[BUFFER_SIZE] = {0};
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }
    
    while (1) {
        char filename[100];
        char key[27];
        
        // Get filename from user
        while (1) {
            printf("Enter the filename to encrypt: ");
            scanf("%s", filename);
            
            // Check if file exists
            FILE *file = fopen(filename, "r");
            if (!file) {
                printf("NOTFOUND %s\n", filename);
                continue;
            }
            fclose(file);
            break;
        }
        
        // Get encryption key from user
        while (1) {
            printf("Enter the 26-character encryption key: ");
            scanf("%s", key);
            
            if (!verify_key(key)) {
                printf("Invalid key. Key must be exactly 26 characters.\n");
                continue;
            }
            break;
        }
        
        // Send key to server
        if (send(sock, key, 26, 0) != 26) {
            printf("Error sending key\n");
            close(sock);
            return -1;
        }
        
        // Send file content to server in chunks
        FILE *file = fopen(filename, "r");
        while (1) {
            memset(chunk, 0, CHUNK_SIZE);
            size_t bytes_read = fread(chunk, 1, CHUNK_SIZE, file);
            if (bytes_read <= 0) break;
            
            // Send this chunk
            size_t bytes_sent = 0;
            while (bytes_sent < bytes_read) {
                int sent = send(sock, chunk + bytes_sent, bytes_read - bytes_sent, 0);
                if (sent <= 0) {
                    printf("Error sending file chunk\n");
                    fclose(file);
                    close(sock);
                    return -1;
                }
                bytes_sent += sent;
            }
            
            usleep(10000);  // 10ms delay between chunks
        }
        fclose(file);
        
        // Send end-of-file marker (empty chunk)
        send(sock, "", 1, 0);
        
        // Create encrypted filename
        char enc_filename[100];
        snprintf(enc_filename, sizeof(enc_filename), "%s.enc", filename);
        
        // Receive encrypted file from server in chunks
        FILE *enc_file = fopen(enc_filename, "w");
        while (1) {
            memset(recv_buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(sock, recv_buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) break;
            
            size_t bytes_written = 0;
            while (bytes_written < bytes_received) {
                size_t written = fwrite(recv_buffer + bytes_written, 1, 
                                     bytes_received - bytes_written, enc_file);
                if (written <= 0) {
                    printf("Error writing to encrypted file\n");
                    fclose(enc_file);
                    close(sock);
                    return -1;
                }
                bytes_written += written;
            }
        }
        fclose(enc_file);
        
        printf("File encrypted successfully!\n");
        printf("Original file: %s\n", filename);
        printf("Encrypted file: %s\n", enc_filename);
        
        // Ask if user wants to encrypt another file
        char continue_choice[10];
        printf("Do you want to encrypt another file? (Yes/No): ");
        scanf("%s", continue_choice);
        
        if (strcasecmp(continue_choice, "No") == 0) {
            break;
        }
    }
    
    close(sock);
    return 0;
}