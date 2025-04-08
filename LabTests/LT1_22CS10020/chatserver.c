//Name: Dadi Sasank Kumar
//Rno: 22CS10020

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<ctype.h>


#define buffer_size 1024
#define port 8080

int main()
{
    int server_fd,client_fd;
    // int client_fd[5];
    int num;
    struct sockaddr_in client_addr,server_addr;
    char buffer[buffer_size];
    int numbers[5];
    int numclient=0;
    socklen_t client_len=sizeof(client_addr);


    server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }


    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Binding failed.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd,5)<0){
        perror("Listen failed.\n");
        exit(EXIT_FAILURE);
    }

    for(int i=0 ; i<5 ; i++){
        numbers[i]=0; //initialization
    }

    int i=0;
    
    while(1){ 
    
    
    printf("TCP Server listening on port: %d\n",port);
    client_fd=accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
    if(client_fd<0){
        perror("Accept failed.\n");
        exit(EXIT_FAILURE);
    }

    
    int n=read(client_fd,buffer,buffer_size);
    buffer[n]='\0';
    // printf("IP Adress: %s\n",buffer);
    
    char *ip=buffer;
    printf("Recieved a new connection from client %s : ",buffer);
    numclient+=1;
    printf("\nnumber of clients: %d\n",numclient);

    int m=read(client_fd,buffer,buffer_size);
    buffer[m]='\0';
    printf("%s\n\n",buffer);

    // char *port_no=buffer;

    // printf("%s\n",port_no);


    char *msg="Send your number for Round <n>\n";
    write(client_fd,msg,strlen(msg));
    // sleep(1);
    // int num=n;
    // char *num_a="4";
    // int rou;
    // write(client_fd,&round,sizeof(num_a));


    int numbe;
    int o=read(client_fd,&numbe,sizeof(int));
    num=numbe;
    printf("Number recieved from client: %d\n",num);

    

    num+=1;
    i+=1;
    }
    close(server_fd);
    close(client_fd);
    return 0;

}