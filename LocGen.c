/* LocGen (Location Generator) */
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> 
#include <pthread.h>
#define MAX 1024
#define PORT 8080
#define ipServer "127.0.0.1"
#define SA struct sockaddr
#define N 100  //100 sensors

int sockfd;

pthread_t threadID[N + 1];
// Mutex INIT 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int threadRoom_N = -1;
int flag = 0;
long ArrayThreadRoom[N], temp_threadRoom[N];  

char buff[MAX], buff_ID[MAX];

void *Thread_Room(void *arg) {
  //printf("flag1= %d ",flag);

  // Mutex LOCK 
  if (pthread_mutex_lock(&mutex) != 0)
  {
    perror("pthread_mutex_lock() error");
    exit(EXIT_FAILURE);
  }

  int set = 0;
  for (int i = 0; i <= threadRoom_N; i++) {
    temp_threadRoom[i] = ArrayThreadRoom[i];
    //printf("\ntest: %ld\n",temp_threadRoom[i]);
  }
  for (int i = 0; i <= threadRoom_N; i++) {
    if (temp_threadRoom[i] != -1)
      flag = 1;  
  }
  //printf("flag2= %d ",flag);
  for (int i = 0; i <= threadRoom_N; i++) {
    if ((long)arg ==temp_threadRoom[i]) {
      if(flag==1){
        int x = rand() % 10;                                                                                
        int y = rand() % 10;

        char str_id[10], str_x[10], str_y[10];
        sprintf(str_id, "ID=%ld, ",(long)arg);
        sprintf(str_x, "(x,y)=(%d,",x);
        sprintf(str_y, "%d)",y);
        strcat(str_id, str_x);
        strcat(str_id, str_y);
        printf("\nSensor %ld have location: %s\n", (long)arg, str_id);  //ID,x,y
        send(sockfd, str_id, strlen(str_id), 0);             
        set = 1; //setup   
      }                                                                         
    }
  }
  if(set==1){
    flag = 0;
    set = 0;
    for (int i = 0; i <= threadRoom_N; i++) 
      temp_threadRoom[i] = -1;
  }

  // Mutex UNLOCK 
  if (pthread_mutex_unlock(&mutex) != 0)
  {
    perror("pthread_mutex_lock() error");
    exit(EXIT_FAILURE);
  }

  sleep(2); 
}

void *Thread_CLI(void *client_sock) {
  int sockfd = *((int *)client_sock);
  int valread;
  for (int i = 0; i < N; i++) {
    temp_threadRoom[i] = -1;  
    ArrayThreadRoom[i] = -1;    
  }
  while (1) {
    bzero(buff, sizeof(buff));
    if ((valread = read(sockfd, buff, sizeof(buff))) !=0)
    {
      buff[valread] = '\0';
      //fputs(buff,stdout);
      // get ID
      int count = 6;
      while (1) {
        buff_ID[count - 6] = buff[count];
        if (buff[count] == '\0') break;
        count++;
      }

      if ((strncmp(buff, "create", 6)) == 0) {
        char *ptr;
        long check_IDsensor;
        check_IDsensor = strtol(buff_ID, &ptr, 10);// get ID
        threadRoom_N++;  // add thread new sensor
        ArrayThreadRoom[threadRoom_N] = check_IDsensor;

        if (pthread_create(&threadID[threadRoom_N], NULL,(void *)Thread_Room,(void *)check_IDsensor) != 0) {
          printf("Error: pthread_create() failed...\n");
          exit(EXIT_FAILURE);
        } else {
          char strbuff[MAX] = "Create sensor have ID:";
          strcat(strbuff, buff_ID);
          printf("\n%s", strbuff);
          send(sockfd, strbuff, strlen(strbuff), 0);
          flag = 0;  //sleep time is 3s
        }
      } else if ((strncmp(buff, "delete", 6)) == 0) {
        char *ptr;
        long check_IDsensor;
        check_IDsensor = strtol(buff_ID, &ptr, 10); //get ID
        // printf("ID: %ld",check_IDsensor);
        char strbuff[MAX] = "Delete sensor have ID:";
        int flag1 = 0;
        for (int i = 0; i <= threadRoom_N; i++) {
          if (ArrayThreadRoom[i] == check_IDsensor) {
            pthread_cancel(threadID[i]);
            // delete this sensor
            for (int j = i; j < threadRoom_N; j++) {
              ArrayThreadRoom[j] = ArrayThreadRoom[j + 1];
            }
            ArrayThreadRoom[threadRoom_N] = -1;

            // setup temp_threadRoom
            for (int i = 0; i < threadRoom_N; i++)
              temp_threadRoom[i] = ArrayThreadRoom[i]; 
            strcat(strbuff, buff_ID);
            //printf("test: %s",strbuff);
            send(sockfd, strbuff, strlen(strbuff), 0);
            flag = 0;  // //sleep time is 3s
            flag1 = 1;
            break;
          }
        }

        if (flag1 == 0) {
          char str_buff[MAX] = "Sensor does not exist...";
          send(sockfd, str_buff, strlen(str_buff), 0);
        } else  {
          printf("\n%s", strbuff);
          flag = 0;
          threadRoom_N--;  // delete sensor : N-1
        }
      } else if (strncmp("list_3", buff, 6) == 0) {
        char str[MAX] = "List all sensors now: ", str1[MAX]; 
        for (int i = 0; i <= threadRoom_N; i++) {
          sprintf(str1, "%ld ", ArrayThreadRoom[i]);
          strcat(str, str1);
          bzero(str1, MAX);
          // printf("test: %ld",ArrayThreadRoom[i]);
        }
        printf("\n%s\n", str);
        send(sockfd, str, strlen(str), 0);
        flag = 0;  //sleep time is 3s

        bzero(str, MAX);
      } else if (strncmp("exit_0", buff, 6) == 0) {
          break;
        }
      bzero(buff, MAX);
      bzero(buff_ID, MAX);
    }

    flag++;
    sleep(1);
    //pthread_exit(NULL);
  }
}

int main() {
  struct sockaddr_in servaddr;
  srand((int)time(0));
  pthread_t recvCLI;
  
  // socket create and varification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket creation failed...\n");
    exit(EXIT_FAILURE);
  }
  else
    printf("Socket successfully created...\n");
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
    printf("Connected to the server...\n");

  // check connect ...
  read(sockfd, buff, sizeof(buff));
  printf("From Server VTS: %s\n", buff); 

  //creating a client thread which is always waiting for a message
  pthread_create(&recvCLI,NULL,(void *)Thread_CLI,&sockfd);

  //thread is closed
  pthread_join(recvCLI,NULL);

  // close the socket
  close(sockfd);

  exit(EXIT_SUCCESS);;
}

