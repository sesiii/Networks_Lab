#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>

#define server_ip "127.0.0.1"
#define port 8080
#define buffer_size 1024

int main()
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[buffer_size];
    socklen_t addr_len =sizeof(server_addr);

    sock_fd=socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    inet_pton(AF_INET,server_ip,&server_addr.sin_addr);

    char *msg="Heyy server!\nI am doing great!";
    sendto(sock_fd,msg,strlen(msg),0,( struct sockaddr*)&server_addr,sizeof(server_addr));
    

    int m=recvfrom(sock_fd,buffer,buffer_size,0,( struct sockaddr*)&server_addr,&addr_len);
    buffer[m]='\0';
    // sleep(1);
    printf("Server:\n%s",buffer);

    sleep(1);
    printf("Quitting...");
    close(sock_fd);
    return 0;

}
