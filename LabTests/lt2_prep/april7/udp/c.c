#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<error.h>
#include<string.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{

    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len=sizeof(server_addr);

    sock_fd=socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd<0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    //setting server addres
    memset(&server_addr,0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    inet_pton(AF_INET,SERVER_IP, &server_addr.sin_addr);

    
    char *msg="HI server bhai...\n";
    sendto(sock_fd,msg,strlen(msg),0, (struct sockaddr *)&server_addr,sizeof(server_addr));
    printf("Message sent to server.\n");
    
}