#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<signal.h>

#define port 8080
#define BUFFER_SIZE 1024

void handle_client(int client_sock ,char *client_ip,int client_port){
    char buffer[BUFFER_SIZE];
    char str[100];
    int n=read(client_sock,buffer,BUFFER_SIZE);
    buffer[n]='\0';
    snprintf(str,"Received from %s:%d-->%s",client_ip,client_port,buffer);
    write(client_sock,"ACK",3);
    close(client_sock);
}
int main()
{
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in client_addr,server_addr;
    socklen_t addr_len=sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    signal(SIGCHLD,SIG_IGN);

    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    if(bind(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(sock_fd,10)<0){
        perror("Listen failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("TCP server listening on port %d.\n",port);
    while(1){
        int client_sock=accept(sock_fd,(struct sockaddr*)&client_addr,&addr_len);
        if(client_sock<0){
            perror("Accept failed.\n");
            exit(EXIT_FAILURE);
        }

        char client_ip[16];
        inet_ntop(AF_INET,&client_addr.sin_addr,client_ip,16);
        int client_port=ntohs(client_addr.sin_port);
        printf("Client connection from %s:%d\n",client_ip,client_port);

        pid_t pid=fork();
        if(pid==0){
            close(sock_fd);
            handle_client(client_sock,client_ip,client_port);
        }
        else if(pid<0){
            close(sock_fd);
        }
        else{
            perror("Fork failed.\n");
        }
    }

    close(sock_fd);
    return 0;
}