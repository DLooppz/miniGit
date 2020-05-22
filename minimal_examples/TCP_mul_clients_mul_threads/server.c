#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<math.h>

#define SIZE 1024
#define PORT 7000

char client_message[SIZE + 1];
char buffer[SIZE + 1];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int socket_fd;
    struct sockaddr_in clientAddr;
} threadSocketInfo;

void * socketThread(void *arg){
    int n_bytes, opt = 1, dataSocket, commSocket;
    threadSocketInfo * ptr_threadCommSocketInfo, *ptr_threadDataSocketInfo;
    ptr_threadCommSocketInfo = (threadSocketInfo *) arg;
    ptr_threadDataSocketInfo = malloc(sizeof(threadSocketInfo));
    commSocket = ptr_threadCommSocketInfo->socket_fd;
    char *client_message = (char *) malloc(SIZE * sizeof(char));
    char *buffer = (char *) malloc(SIZE * sizeof(char));
    
    printf("Init socketThread - The commands port being used is  %d \n",ptr_threadCommSocketInfo->clientAddr.sin_port);

    int bindRet, listenSocket;
    struct sockaddr_in listenAddr;
    socklen_t listenAddr_size;
    unsigned short int listenPort;

    // Create new socket
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = INADDR_ANY;
    memset(listenAddr.sin_zero, '\0', sizeof(listenAddr.sin_zero));
    listenPort = PORT;
    while(1){
        listenPort ++;
        listenAddr.sin_port = htons(listenPort);
        bindRet = bind(listenSocket, (struct sockaddr *) &listenAddr, sizeof(listenAddr));
        if(bindRet == 0){
            printf("Data listening port: %d\n",listenPort);
            break;
        }
    }
    if(listen(listenSocket,1)==-1)
        printf("Error\n");
    

    // Send welcome mssg and data port (associated with the socket that we just created) to client
    char strPort[10]; //char strPort[(int)((ceil(log10((int)listenPort))+1)*sizeof(char))];  more complicated but gives the precise size
    sprintf(strPort, "%d", listenPort);
    bzero(buffer, sizeof(buffer));
    strcpy(buffer,"Hi! I'm the server.\nYou can send me data through the port:");
    strcat(buffer,strPort);
    strcat(buffer,"--");
    send(commSocket,buffer,strlen(buffer),0);

    // TODO: wait until the client responds with the correct socket (i.e equal to listenPort)

    // TODO: tell the client to send some testing data

    // TODO: accept 

    // By this point we have two sockets (commSocket and dataSocket) both bidirectional

    while(1){
        // TODO: insert commSocket and dataSocket into fd_set
        
        // TODO: select (and so update fd_set)

        // TODO: if commSocket is in fd_set
        //       make some logic

        // TODO: if dataSocket is in fd_set
        //       make some logic


        n_bytes = recv(commSocket , client_message , 1024 , 0);

        printf("%s\n",client_message);
        if(strcmp(client_message,"stop") == 0){
            break;
        }

        strcpy(buffer,"Hello Client : ");
        strcat(buffer,client_message);
        strcat(buffer,"\0");
        send(commSocket,buffer,strlen(buffer),0);


        bzero(buffer, sizeof(buffer));
        bzero(client_message, sizeof(client_message));
    }

    // new
    bzero(buffer, sizeof(buffer));
    bzero(client_message, sizeof(client_message));
    listenAddr_size = sizeof(ptr_threadDataSocketInfo->clientAddr);
    ptr_threadDataSocketInfo->socket_fd = accept(listenSocket, (struct sockaddr *) &(ptr_threadDataSocketInfo->clientAddr), &listenAddr_size);
    dataSocket = ptr_threadDataSocketInfo->socket_fd;
    printf("-----------\nThrough port %d\n-----------\n",ptr_threadDataSocketInfo->clientAddr.sin_port);
    n_bytes = recv(dataSocket , client_message , 1024 , 0);
    printf("%s\n",client_message);
    strcpy(buffer,"Bye Client through data port: ");
    strcat(buffer,client_message);
    strcat(buffer,"\0");
    send(dataSocket,buffer,strlen(buffer),0);
    // new

    // Free memory and close sockets
    printf("Exit socketThread - The ports being used were %d and %d\n",ptr_threadCommSocketInfo->clientAddr.sin_port,ptr_threadDataSocketInfo->clientAddr.sin_port);
    free(ptr_threadCommSocketInfo);
    free(ptr_threadDataSocketInfo);
    free(buffer);
    free(client_message);
    close(listenSocket);
    close(commSocket);
    close(dataSocket);

    // End thread
    pthread_exit(NULL);
}

int main(){
    int opt = 1;
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in newSocketAddr;
    socklen_t addr_size;

    //Create the socket. 
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));

    // Configure settings of the server address struct
    // Address family = Internet 
    serverAddr.sin_family = AF_INET;

    //Set port number, using htons function to use proper byte order 
    serverAddr.sin_port = htons(PORT);

    //Set IP address to localhost 
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    //Set all bits of the padding field to 0 
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Bind the address struct to the socket 
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    //Listen on the socket, with 50 max connection requests queued 
    if(listen(serverSocket,50)==0)
        printf("Listening\n");
    else
        printf("Error\n");

    threadSocketInfo * ptr_threadCommSocketInfo;
    pthread_t tid[50];
    int i = 0;
    addr_size = sizeof(newSocketAddr);
    
    while(1){
        //Accept call creates a new socket for the incoming connection
        newSocket = accept(serverSocket, (struct sockaddr *) &newSocketAddr, &addr_size);
        //for each client request creates a thread and assign the client request to it to process
        //so the main thread can entertain next request

        ptr_threadCommSocketInfo = malloc(sizeof(threadSocketInfo));
        ptr_threadCommSocketInfo->socket_fd = newSocket;
        ptr_threadCommSocketInfo->clientAddr = newSocketAddr;
        // ptr_threadCommSocketInfo->clientAddr = malloc(sizeof(struct sockaddr_in));
        // ptr_threadCommSocketInfo->clientAddr->sin_family = newSocketAddr.sin_family;
        // ptr_threadCommSocketInfo->clientAddr->sin_port = newSocketAddr.sin_port;
        // ptr_threadCommSocketInfo->clientAddr->sin_addr.s_addr= newSocketAddr.sin_addr.s_addr;
        // memset(ptr_threadCommSocketInfo->clientAddr->sin_zero, '\0', sizeof ptr_threadCommSocketInfo->clientAddr->sin_zero);

        if( pthread_create(&tid[i], NULL, socketThread, ptr_threadCommSocketInfo) != 0 )
            printf("Failed to create thread\n");
        
        if( i >= 40){
            i = 0;
            while(i < 50){
                pthread_join(tid[i++],NULL);
            }
            i = 0;
        }
        i++;
        
    }
    return 0;
}