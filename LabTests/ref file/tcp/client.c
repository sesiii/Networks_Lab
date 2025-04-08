#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

#define port 8080
#define BUFFER_SIZE 1024
int main()
{
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    socklen_t addr_len=sizeof(server_addr);

    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    connect(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr));
    while(1){

        char *msg="hi ra...\n";
        char s[BUFFER_SIZE];
        printf("Enter message to send: ");
        fgets(s, BUFFER_SIZE, stdin);
        
        write(sock_fd,s,strlen(s));
        // usleep(2);
    }
    close(sock_fd);

}