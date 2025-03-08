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
  * Receiver application for testing KTP sockets
  * This program receives file data from a sender using the KTP protocol
  * and writes it to a file
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
     
     DEBUG("Starting receiver with local_ip=%s:%d, remote_ip=%s:%d", 
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
     
     // Open file to write received data
     const char *filename = "receivedfile.txt";
     FILE *file = fopen(filename, "wb");
     if (!file) {
         perror("Failed to open output file");
         DEBUG("Failed to create output file %s", filename);
         k_close(ktp_sock);
         return 1;
     }
     DEBUG("Opened output file %s for writing", filename);
     
     // Receive the file
     char buffer[FILE_CHUNK_SIZE];
     size_t total_bytes_received = 0;
     int chunks_received = 0;
     int consecutive_timeouts = 0;
     const int max_timeouts = 50;  // Exit after this many consecutive timeouts (5 seconds)
     
     printf("Receiving file...\n");
     while (consecutive_timeouts < max_timeouts) {
         ssize_t received = k_recvfrom(ktp_sock, buffer, FILE_CHUNK_SIZE, 0);
         if (received < 0) {
             if (ktp_errno == ENOMESSAGE) {
                 DEBUG("No message available yet for socket %d, waiting...", ktp_sock);
                 usleep(100000);  // 100ms
                 consecutive_timeouts++;
                 continue;
             } else {
                 perror("k_recvfrom failed");
                 DEBUG("k_recvfrom failed for socket %d, ktp_errno=%d", ktp_sock, ktp_errno);
                 break;
             }
         }
         
         consecutive_timeouts = 0;  // Reset timeout counter when we receive data
         
         // Write received data to file
         size_t bytes_written = fwrite(buffer, 1, received, file);
         if (bytes_written != (size_t)received) {
             perror("Failed to write to file");
             DEBUG("Write to file failed, expected=%zu, written=%zu", received, bytes_written);
             break;
         }
         
         total_bytes_received += received;
         chunks_received++;
         
         printf("\rReceived: %zu bytes in %d chunks", total_bytes_received, chunks_received);
         fflush(stdout);
         DEBUG("Received chunk %d for socket %d, bytes=%zu, total=%zu", 
               chunks_received, ktp_sock, received, total_bytes_received);
         
         // Optional: Stop after receiving expected file size (e.g., 100KB)
         if (total_bytes_received >= 100 * 1024) {
             DEBUG("Reached expected file size (100KB), stopping");
             break;
         }
     }
     
     if (consecutive_timeouts >= max_timeouts) {
         printf("\nTransfer appears to be complete (no data received for 5 seconds)\n");
     }
     
     printf("\nFile reception completed: %zu bytes in %d chunks\n", total_bytes_received, chunks_received);
     DEBUG("File reception completed, total bytes=%zu, chunks=%d", total_bytes_received, chunks_received);
     
     // Flush and close file
     fflush(file);
     fclose(file);
     DEBUG("Closed output file %s", filename);
     
     if (k_close(ktp_sock) < 0) {
         perror("k_close failed");
     } else {
         printf("Socket closed\n");
         DEBUG("Socket %d closed", ktp_sock);
     }
     
     return 0;
 }