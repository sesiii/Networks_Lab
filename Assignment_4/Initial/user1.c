/**
 * =====================================
 * Assignment 4 Submission
 * Name: [Your_Name]
 * Roll number: [Your_Roll_Number]
 * Link of the pcap file: [Google_Drive_Link_of_the_pcap_File]
 * =====================================
 */

 #include "ksocket.h"
 #include <stdarg.h>
 
 #define FILE_CHUNK_SIZE MAX_MSG_SIZE
 #define DEBUG(fmt, ...) printf("[DEBUG %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
 
 /**
  * Sender application for testing KTP sockets
  * This program creates a test file and sends it to a receiver using the KTP protocol
  */
 int main(int argc, char *argv[]) {
     if (argc != 5) {
         fprintf(stderr, "Usage: %s <local_ip> <local_port> <remote_ip> <remote_port>\n", argv[0]);
         return 1;
     }
     
     const char *local_ip = argv[1];
     int local_port = atoi(argv[2]);
     const char *remote_ip = argv[3];
     int remote_port = atoi(argv[4]);
     
     DEBUG("Starting sender with local_ip=%s:%d, remote_ip=%s:%d", 
           local_ip, local_port, remote_ip, remote_port);
 
     // Create a KTP socket
     int ktp_sock = k_socket(AF_INET, SOCK_KTP, 0);
     if (ktp_sock < 0) {
         perror("k_socket failed");
         DEBUG("Failed to create KTP socket, errno=%d", errno);
         return 1;
     }
     printf("KTP socket created: %d\n", ktp_sock);
     DEBUG("KTP socket created successfully, id=%d", ktp_sock);
     
     // Prepare addresses
     struct sockaddr_in src_addr, dst_addr;
     memset(&src_addr, 0, sizeof(src_addr));
     src_addr.sin_family = AF_INET;
     src_addr.sin_port = htons(local_port);
     if (inet_pton(AF_INET, local_ip, &src_addr.sin_addr) <= 0) {
         perror("inet_pton failed for local address");
         k_close(ktp_sock);
         return 1;
     }
     
     memset(&dst_addr, 0, sizeof(dst_addr));
     dst_addr.sin_family = AF_INET;
     dst_addr.sin_port = htons(remote_port);
     if (inet_pton(AF_INET, remote_ip, &dst_addr.sin_addr) <= 0) {
         perror("inet_pton failed for destination address");
         k_close(ktp_sock);
         return 1;
     }
     
     // Bind the socket
     if (k_bind(ktp_sock, (struct sockaddr *)&src_addr, sizeof(src_addr),
                (struct sockaddr *)&dst_addr, sizeof(dst_addr)) < 0) {
         perror("k_bind failed");
         DEBUG("Binding failed for socket %d, errno=%d", ktp_sock, errno);
         k_close(ktp_sock);
         return 1;
     }
     printf("Socket bound to %s:%d -> %s:%d\n", local_ip, local_port, remote_ip, remote_port);
     DEBUG("Socket %d bound successfully", ktp_sock);
     
     // Create or open a file to send
     const char *filename = "testfile.txt";
     FILE *file = fopen(filename, "rb");
     if (!file) {
         printf("Creating test file: %s\n", filename);
         DEBUG("Test file %s not found, creating it", filename);
         file = fopen(filename, "wb");
         if (!file) {
             perror("Failed to create test file");
             DEBUG("Failed to create test file %s", filename);
             k_close(ktp_sock);
             return 1;
         }
         
         // Write data to the file (100KB)
         const size_t size = 100 * 1024;
         char *data = (char *)malloc(size);
         if (!data) {
             perror("Failed to allocate memory");
             DEBUG("Memory allocation failed for test file data");
             fclose(file);
             k_close(ktp_sock);
             return 1;
         }
         
         for (size_t i = 0; i < size; i++) {
             data[i] = (char)(i % 26 + 'a');
         }
         
         fwrite(data, 1, size, file);
         fclose(file);
         free(data);
         DEBUG("Created test file %s with %zu bytes", filename, size);
         
         file = fopen(filename, "rb");
         if (!file) {
             perror("Failed to open test file");
             DEBUG("Failed to reopen test file %s", filename);
             k_close(ktp_sock);
             return 1;
         }
     }
     
     // Get file size
     fseek(file, 0, SEEK_END);
     long file_size = ftell(file);
     fseek(file, 0, SEEK_SET);
     
     printf("Sending file: %s (%ld bytes)\n", filename, file_size);
     DEBUG("Preparing to send file %s, size=%ld bytes", filename, file_size);
     
     // Send the file
     char buffer[FILE_CHUNK_SIZE];
     size_t bytes_read;
     size_t total_bytes_sent = 0;
     int chunks_sent = 0;
     
     while ((bytes_read = fread(buffer, 1, FILE_CHUNK_SIZE, file)) > 0) {
         ssize_t sent = k_sendto(ktp_sock, buffer, bytes_read, 0);
         if (sent < 0) {
             if (ktp_errno == ENOSPACE) {
                 DEBUG("Send buffer full for socket %d, waiting...", ktp_sock);
                 usleep(100000);  // 100ms
                 continue;
             } else {
                 perror("k_sendto failed");
                 DEBUG("k_sendto failed for socket %d, ktp_errno=%d", ktp_sock, ktp_errno);
                 break;
             }
         }
         
         total_bytes_sent += bytes_read;
         chunks_sent++;
         
         printf("\rSent: %.2f%% (%zu/%ld bytes, %d chunks)", 
                (double)total_bytes_sent / file_size * 100, 
                total_bytes_sent, file_size, chunks_sent);
         fflush(stdout);
         DEBUG("Sent chunk %d for socket %d, bytes=%zu, total=%zu", 
               chunks_sent, ktp_sock, bytes_read, total_bytes_sent);
         
         usleep(10000);  // 10ms rate limit to avoid overloading the receiver
     }
     
     printf("\nFile transfer completed: %zu bytes in %d chunks\n", total_bytes_sent, chunks_sent);
     DEBUG("File transfer completed, total bytes=%zu, chunks=%d", total_bytes_sent, chunks_sent);
     
     // Clean up
     fclose(file);
     
     printf("Waiting for all messages to be sent...\n");
     DEBUG("Waiting 10s to ensure all messages are processed");
     sleep(10);  // Wait to ensure all messages are sent before closing
     
     if (k_close(ktp_sock) < 0) {
         perror("k_close failed");
     } else {
         printf("Socket closed\n");
         DEBUG("Socket %d closed", ktp_sock);
     }
     
     return 0;
 }