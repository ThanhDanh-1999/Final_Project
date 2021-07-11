/* CLI Client */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX 256
#define PORT 8080
#define ipServer "127.0.0.1"
#define SA struct sockaddr

char buff[MAX];
char send_buff[MAX];

// function CLI Client for table console
void func(int sockfd)
{
    while(1) {
    printf("\t\t\t ==========  VEHICLE TRACKING SYSTEM  ==========\n");
    printf("\t\t\t |1. Create new sensor                         |\n");
    printf("\t\t\t |2. Delete sensor                             |\n");
    printf("\t\t\t |3. Display list                              |\n");
    printf("\t\t\t |0. Exit                                      |\n");
    printf("\t\t\t ===============================================\n");
    printf("Enter your choice [1-4]: ");
    bzero(buff, sizeof(buff));
    fgets(buff,sizeof(buff),stdin);

    int n = 0;
    if (strncmp(buff, "1", 1) == 0) {
        bzero(buff, sizeof(buff));
        printf("Enter ID of new sensor : ");
        while ((buff[n++] = getchar()) != '\n')
            ;
        strcpy(send_buff, "create");
        strcat(send_buff," ");
        strcat(send_buff,buff);

    } else if (strncmp(buff, "2", 1) == 0) {
        bzero(buff, sizeof(buff));
        printf("Enter ID of delete sensor: ");
        while ((buff[n++] = getchar()) != '\n')
            ;
        strcpy(send_buff, "delete");
        strcat(send_buff," ");
        strcat(send_buff,buff);
    } else if (strncmp(buff, "3", 1) == 0) {
        bzero(send_buff, sizeof(send_buff));
        strcpy(send_buff, "list");
        strcat(send_buff,"_");
        strcat(send_buff,buff);
    } else if (strncmp(buff, "0", 1) == 0) {
        bzero(send_buff, sizeof(send_buff));
        strcpy(send_buff, "exit");
        strcat(send_buff,"_");
        strcat(send_buff,buff);
    }

    if(write(sockfd,send_buff,strlen(send_buff)) < 0) 
      perror("client can not send... \n");

    if ((strncmp(buff, "0", 1)) == 0) {
      printf("Client Exit...\n");
      break;
    }
  }
}

int main(int argc,char *argv[]){
    int sockfd;
    struct sockaddr_in servaddr;
  
    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
  
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ipServer);
    servaddr.sin_port = htons(PORT);
  
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        perror("connection with the server failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("connected to the server..\n");
  
    //ready to write a request from console
    func(sockfd);

    // close the socket
    close(sockfd);

    exit(EXIT_SUCCESS);
}

