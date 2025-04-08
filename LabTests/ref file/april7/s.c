#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#define port 8081
#define BUFFER_SIZE 1024
void handle_client(int client_sock,char *client_ip, int client_port){
    char buffer[BUFFER_SIZE];
    int n=read(client_sock,buffer,BUFFER_SIZE);
    buffer[n]='\0';
    char str[10000];
    sprintf(str,"Client(%s:%d) : %s\n",client_ip,client_port,buffer);
    printf("%s",str);
    write(client_sock,"ACK",3);
    printf("ACK sent\n");
    write(client_sock,"ACK1",3);
    printf("ACK1 sent\n");
    close(client_sock);
    exit(0);
}
int main()
{
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0){
        //
        perror("socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr,client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len=sizeof(client_addr);

    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    if(bind(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        //bind failed
        perror("socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    if(listen(sock_fd,10)<0){
        // listen failed
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n",port);
    while(1){
        int client_sock=accept(sock_fd,(struct sockaddr*)&client_addr,&addr_len);
        if(client_sock<0){
            //
            perror("client sock failed\n");
        exit(EXIT_FAILURE);
        }
        char client_ip[16];
        inet_ntop(AF_INET,&client_addr.sin_addr,client_ip,16);
        int client_port=ntohs(client_addr.sin_port);
        printf("\nNew connection from %s:%d\n",client_ip,client_port);

        pid_t pid=fork();
        printf("Forking to handle client...\n");
        if(pid==0){
            close(sock_fd);
            handle_client(client_sock,client_ip,client_port);
        }
        else if(pid>0){
            close(client_sock);
        }
        else{
            perror("fork failed.\n");
        }
    }
    close(sock_fd);
    return 0;
}