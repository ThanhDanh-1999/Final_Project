/* VTS (Vehicle Tracking System) */
#include <arpa/inet.h> 
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h>
#include <sys/time.h> /* For portability */
#include <sys/select.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>  
#include <pthread.h>
#include "tinyxml2.cpp" /* Embedded tinyxml2 */
#include "tinyxml2.h"
#define MAX 1024
#define PORT 8080
#define SA struct sockaddr
#define N 100  //100 sensors

struct sockaddr_in servaddr;
fd_set readfds;
int sockfd, sock, valAddress, valread, client_socket[10], max_clients = 2;

char buff[MAX], send_buff[MAX], data_buff[MAX];

pthread_t threadID1, threadID2, threadID3, threadID4;
// mutex INIT 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

long ArrayThreadRoom[N]; 
int threadRoom_N = 0;
int flag = 0;

int fd[2], count_data = 0; // pipe

struct GetCoords{
  	char ID[16] = "";
	char X[2] = "";
	char Y[2] = "";
};
struct GetCoords Coord[] = {};

/* Thread receive commands from the keyboard */
void *Thread_Recv_CLI(void *arg) {
  while (1) {
    sock = client_socket[1];  // setup CLI client
	
	  //Check if the file descriptor to track is in the fdset setset
    if (FD_ISSET(sock, &readfds)) {
      if ((valread = read(sock, buff, MAX)) == 0) 
      {
        // get name of connected peer socket
        getpeername(sock, (SA*)&servaddr, (socklen_t *)&valAddress);
        printf("Disconnected , ip %s , port %d \n",inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

        // Close the socket
        close(sock);
        client_socket[1] = 0;
      }

      else {
        buff[valread] = '\0'; // add a null character to the end to make a string
        if (strncmp("create", buff, 6) == 0 || strncmp("delete", buff, 6) == 0)
          strcpy(send_buff, buff);
        else if (strncmp("list_3", buff, 6) == 0)
          strcpy(send_buff, buff);
        else if (strncmp("exit_0", buff, 6) == 0) {
          strcpy(send_buff, buff);
          printf("Server VTS Exit...\n");  
		      flag = 1;
          pthread_exit(NULL);  
		    }
      }

      bzero(buff, MAX);
    }
    sleep(1);
  }
}

/* Thread send commands to Client */
void *Thread_CLItoLocGen(void *arg)
{
  while (1) {
    if (strlen(send_buff) > 0) {
      send(client_socket[0], send_buff, strlen(send_buff), 0);
    }
    if (flag == 1){ 
		//printf("Server VTS Exit...\n"); 
		pthread_exit(NULL); 
	}
    bzero(send_buff, MAX);
    sleep(1);
  }
}


void openPipe(void) {
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, fd) < 0) {
    fprintf(stderr, "File : %s , Line : %d , socketpair error:", __FILE__, __LINE__);
    perror("");
    abort();
  }
}
void writePipe() {
  int iRetSts;
  iRetSts = write(fd[1], data_buff, sizeof(data_buff));
  if (iRetSts == -1) {
    fprintf(stderr, "File : %s , Line : %d , write failed \n", __FILE__, __LINE__);
    perror("");
  } else {
    count_data++;
    //printf("\ncount: %d",count_data);
  }
}

void writexml(){
	/**************************************
	<?xml version="1.0" encoding="UTF-8"?>
	<!--Wellcome to Vehicle Tracking System-->
	<Sensors>
	<ID>number of id</ID>
	<Data logically>
		<X>data of x</X>
		<Y>data of y</Y>
	</Data logically>
	</Sensors>
	**************************************/

	// create an instance of XMLDocument
	tinyxml2::XMLDocument doc;
	
	if (count_data >= threadRoom_N) {
		//printf("\nwrite file ID= %s, X= %s, Y= %s\n",Coord[0].ID,Coord[0].X,Coord[0].Y);
		
		doc.LinkEndChild(doc.NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\""));
		doc.LinkEndChild(doc.NewComment("Wellcome to Vehicle Tracking System"));
	 
		auto htmlElement = doc.NewElement("Sensors");
		auto headElement = doc.NewElement("ID");
		headElement->SetText(Coord[0].ID);
		auto bodyElement = doc.NewElement("Data logically");
	 
		htmlElement->LinkEndChild(headElement);
		htmlElement->LinkEndChild(bodyElement);
	 
	 
		auto pElement = doc.NewElement("X");
		pElement->SetText(Coord[0].X);
		auto h1Element = doc.NewElement("Y");
		h1Element->SetText(Coord[0].Y);
	 
		bodyElement->LinkEndChild(pElement);
		bodyElement->LinkEndChild(h1Element);
	 
		doc.LinkEndChild(htmlElement);
	 
		tinyxml2::XMLPrinter printer;
		doc.Print(&printer);
		
		// save the XML-FILE
		doc.SaveFile("config.xml");
	}
    count_data--; // pipe -> count - 1
}


/* Thread receive data from Thread Room of Client and process */
void *Thread_Recv_Data(void *arg) {
  while (1) {
	  // Initialize readfds collection and add socket file descriptor to readfds
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    int newsock = sockfd;

    // add child sockets to set
    for (int i = 0; i < max_clients; i++) {
      sock = client_socket[i];
      if (sock > 0) FD_SET(sock, &readfds);
	  // high number of file descriptors, use select()
      if (sock > newsock) newsock = sock;
    }

	  // Set of socket file descriptors writefds and exceptfds passed NULL
    int temp = select(newsock + 1, &readfds, NULL, NULL, NULL);

    if ((temp < 0) && (errno != EINTR)) {
      perror("ERROR: select() failed...\n");
    }
	
	  /*
	  Check if socket file descriptor is in readfds
	  If FD_ISSET returns 1, socket file descriptor is in readfds and socket file descriptor is ready to read
	  */
    if (FD_ISSET(sockfd, &readfds)) {
	    int client_sock;
      if ((client_sock = accept(sockfd, (SA*)&servaddr, (socklen_t *)&valAddress)) < 0) {
        perror("server acccept failed...\n");
        exit(EXIT_FAILURE);
      } 
      printf("Got connect from socketfd  %d , ip : %s , port : %d\n", client_sock, inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

	    char checkbuff[50] = "Connect to Vehicle Tracking System \r\n"; // check connect to LocGen
      if (send(client_sock, checkbuff, strlen(checkbuff), 0) < 0) {
        perror("sending failure... \n");
      }
      printf("Connected successfully... \n");

      // add new socket for connect
      for (int i = 0; i < max_clients; i++) {
        if (client_socket[i] == 0) {
          client_socket[i] = client_sock;
          //printf("test socket is %d\n", i);
          break;
        }
      }
    } 

    sock = client_socket[0];  // setup to Loc Gen
	  //Check if the file descriptor to track is in the fdset setset
    if (FD_ISSET(sock, &readfds)) {
      if ((valread = read(sock, data_buff, MAX)) == 0)  
      {
        // get name of connected peer socket
        getpeername(sock, (SA*)&servaddr, (socklen_t *)&valAddress);
        printf("Disconnected , ip %s , port %d \n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

        // Close the socket 
        close(sock);
        client_socket[0] = 0;
      }

      else {
        data_buff[valread] = '\0'; // add a null character to the end to make a string
	// get ID
        int count = 22;
        char buff_ID[MAX];
        while (1) {
          buff_ID[count - 22] = data_buff[count];
          if (data_buff[count] == '\0') 
		break;
          count++;
        }
        if (strncmp("Create sensor have ID:", data_buff, 22) == 0) {
          char *ptr;
          long check_IDsensor;
          check_IDsensor = strtol(buff_ID, &ptr, 10); // get ID
          ArrayThreadRoom[threadRoom_N] = check_IDsensor;
          threadRoom_N++; // add thread sensor
         
          setlogmask(LOG_UPTO(LOG_NOTICE));
          openlog("Vehicle Tracking System", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
          syslog(LOG_NOTICE, "%s", data_buff);
          closelog();
		  
          printf("\n%s", data_buff);
        } else if (strncmp("Delete sensor have ID:", data_buff, 22) == 0) {
          char *ptr;
          long check_IDsensor;
          check_IDsensor = strtol(buff_ID, &ptr, 10); // get ID
          // delete this sensor in ArrayThreadRoom N
          for (int i = 0; i < threadRoom_N; i++) {
            if (check_IDsensor == ArrayThreadRoom[i])  
            {
              // delete this sensor
              for (int j = i; j < threadRoom_N; j++) {
                ArrayThreadRoom[j] = ArrayThreadRoom[j + 1];  
			        }
              ArrayThreadRoom[threadRoom_N - 1] = -1;
            }
            break;
          }
          threadRoom_N--; // delete sensor : N-1
          // printf("test id: %ld",check_IDsensor);
		  
          setlogmask(LOG_UPTO(LOG_NOTICE));
          openlog("Vehicle Tracking System", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
          syslog(LOG_NOTICE, "%s", data_buff);
          closelog();
		  
          printf("\n%s", data_buff);
        }  else if (strncmp("List all sensors now:", data_buff, 20) == 0) {
                  printf("\n%s\n", data_buff);
        } else if (strncmp("Sensor does not exist...", data_buff, 24) == 0)
		  printf("\n%s", data_buff);
        else
          writePipe();

        bzero(data_buff, MAX);
      }
    }
    if (flag == 1) pthread_exit(NULL);  
    sleep(0.5);
  }  
}

/* Thread process and store data logically */
void *Thread_Get_Coords(void *arg) {
  char id,x,y; //Get Coords
  while (1) {
    if (count_data >= threadRoom_N) {
      for (int i = 0; i < threadRoom_N; i++)  // recv data
      {
        char temp[18] = "";
        if (read(fd[0], temp, sizeof(temp)) <= 1) break;
        //printf("\n Get coords: %s \n", temp);

        for(int i=0;i<strlen(temp);i++){
          if(i==3){
            id = temp[i];
            char Str_id[4]={id, '\0'};
            strcpy(Coord[0].ID, Str_id);
          } else if(i==13){
                    x = temp[i];
                    char Str_x[4]={x, '\0'};
                    strcpy(Coord[0].X, Str_x);
                  } else if(i==15){
                            y = temp[i];
                            char Str_y[4]={y, '\0'};
                            strcpy(Coord[0].Y, Str_y);
                          }
        }
        bzero(temp, sizeof(temp));
	//printf("\nID= %s, X= %s, Y= %s\n",Coord[0].ID,Coord[0].X,Coord[0].Y);
        // write to xml
        writexml();
        /*
        ID=7, (x,y)=(3,7)
        len -> 18
        id -> 5
        x -> 15
        y -> 17
        */
      }
    }
    if (flag == 1) pthread_exit(NULL);  
    sleep(0.5);
  }	
}

int main(int argc, char *argv[]) {
  openPipe();
  for (int i = 0; i < 80; i++) ArrayThreadRoom[i] = -1;  // no sensor -> -1
  // initialise all client_socket[] to 0 so not checked
  for (int i = 0; i < max_clients; i++) {
    client_socket[i] = 0;
  }

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("ERROR: Socket creation failed...\n");
    exit(EXIT_FAILURE);
  }
  else
    printf("Socket successfully created...\n");
  bzero(&servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);
  
  int option = 1;
  if(setsockopt(sockfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
    perror("ERROR: Setsockopt failed...");
    return EXIT_FAILURE;
  }
	
  // Binding newly created socket to given IP and verification
  if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
    perror("ERROR: Socket binding failed...");
    return EXIT_FAILURE;
  }
  else
    printf("Socket successfully binded...\n");
	
  // Now server is ready to listen and verification
  if ((listen(sockfd, 5)) != 0) {
    perror("ERROR: Socket listening failed...");
    return EXIT_FAILURE;
  }
  else
    printf("Server VTS listening...\n");
	valAddress = sizeof(servaddr);

  printf("\nBefore Thread...\n");
  //Thread Recv CLI
  if (pthread_create(&threadID1, NULL, Thread_Recv_CLI, NULL) != 0) {
    printf("Error: pthread_create() failed...\n");
    exit(EXIT_FAILURE);
  }

  //Thread CLI to LocGen
  if (pthread_create(&threadID2, NULL, Thread_CLItoLocGen, NULL) != 0) {
    printf("Error: pthread_create() failed...\n");
    exit(EXIT_FAILURE);
  }
  
  //Thread Recv data
  if (pthread_create(&threadID3, NULL, Thread_Recv_Data, NULL) != 0) {
    printf("Error: pthread_create() failed...\n");
    exit(EXIT_FAILURE);
  }

  //Thread Get Coords
  if (pthread_create(&threadID4, NULL, Thread_Get_Coords, NULL) != 0) {
    printf("Error: pthread_create() failed...\n");
    exit(EXIT_FAILURE);
  }

  pthread_join(threadID1, NULL);
  pthread_join(threadID2, NULL);
  pthread_join(threadID3, NULL);
  pthread_join(threadID4, NULL);
  printf("\nAfter Thread...\n");

  exit(EXIT_SUCCESS);
}
