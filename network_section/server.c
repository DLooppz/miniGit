// Run with:
// clear && gcc -pthread server.c -lm -o server && ./server

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/select.h>
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
    int sock_fd;
    struct sockaddr_in sock_addr;
} socketInfo_t;

void error(const char *msg) {
    perror(msg);
    exit(0);
}

void * socketThread(void *arg){
    int n_bytes, opt = 1, dataSocket, commSocket;
    socketInfo_t * ptr_threadCommSocketInfo, *ptr_threadDataSocketInfo;
    ptr_threadCommSocketInfo = (socketInfo_t *) arg;
    ptr_threadDataSocketInfo = malloc(sizeof(socketInfo_t));
    commSocket = ptr_threadCommSocketInfo->sock_fd;
    char *client_message = (char *) malloc(SIZE * sizeof(char));
    char *buffer = (char *) malloc(SIZE * sizeof(char));
    
    printf("New thread created\n");
    printf("Commands socket port: %d\n",ptr_threadCommSocketInfo->sock_addr.sin_port);

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
        error("ERROR Listen failed");
    

    // Send welcome mssg and data port (associated with the socket that we just created) to client
    int strPort_size = (int)((ceil(log10((int)listenPort))+1)*sizeof(char));
    char strPort[strPort_size]; // gives the precise size
    snprintf(strPort, strPort_size, "%d", listenPort);
    bzero(buffer, sizeof(buffer));
    strcpy(buffer,strPort);
    send(commSocket,buffer,strlen(buffer),0);

    // TODO: wait until the client responds with the correct socket (i.e equal to listenPort)
    bzero(buffer, sizeof(buffer));
    bzero(client_message, sizeof(client_message));
    listenAddr_size = sizeof(ptr_threadDataSocketInfo->sock_addr);
    ptr_threadDataSocketInfo->sock_fd = accept(listenSocket, (struct sockaddr *) &(ptr_threadDataSocketInfo->sock_addr), &listenAddr_size);
    dataSocket = ptr_threadDataSocketInfo->sock_fd;
    printf("Data socket port: %d\n",ptr_threadDataSocketInfo->sock_addr.sin_port);
    if(dataSocket == -1)
        error("ERROR in accepting listen socket");
    // By this point we have two sockets (commSocket and dataSocket) both bidirectional
    
    fd_set read_fds;
    int max_sfd = commSocket;
    if (dataSocket > max_sfd)
        max_sfd = dataSocket;

    while(1){
        // TODO: insert commSocket and dataSocket into fd_set
        FD_ZERO(&read_fds);
        FD_SET(commSocket, &read_fds);
        FD_SET(dataSocket, &read_fds);

        // TODO: select (and so update fd_set)
        select( max_sfd + 1 , &read_fds , NULL , NULL , NULL);

        // If commSocket is in fd_set, i.e, a command was sent to server
        if(FD_ISSET(commSocket, &read_fds)){
            printf("\n--------StartCommSocket--------\n");
            
            bzero(buffer, sizeof(buffer));
            bzero(client_message, sizeof(client_message));
            
            n_bytes = recv(commSocket , client_message , 1024 , 0);
            printf("%s\n",client_message);
            if(strcmp(client_message,"stop") == 0){
                printf("--------EndCommSocket--------\n");
                break;
            }

            strcpy(buffer,"Hello Client : ");
            strcat(buffer,client_message);
            strcat(buffer,"\0");
            send(commSocket,buffer,strlen(buffer),0);

            printf("--------EndCommSocket--------\n");
        }

        // If dataSocket is in fd_set, i.e, data was sent to server
        if(FD_ISSET(dataSocket, &read_fds)){
            printf("\n--------StartDataSocket--------\n");
            
            bzero(buffer, sizeof(buffer));
            bzero(client_message, sizeof(client_message));
            
            n_bytes = recv(dataSocket , client_message , 1024 , 0);
            printf("%s\n",client_message);
            if(strcmp(client_message,"stop") == 0){
                printf("--------EndDataSocket--------\n");
                break;
            }

            strcpy(buffer,"Hello Client : ");
            strcat(buffer,client_message);
            strcat(buffer,"\0");
            send(dataSocket,buffer,strlen(buffer),0);

            printf("--------EndDataSocket--------\n");
        }
    }

    // Free memory and close sockets
    printf("Exit thread - The ports being used were %d (for commands) and %d (for data)\n",ptr_threadCommSocketInfo->sock_addr.sin_port,ptr_threadDataSocketInfo->sock_addr.sin_port);
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
        error("ERROR in listening server socket");

    socketInfo_t * ptr_threadCommSocketInfo;
    pthread_t tid[50];
    int i = 0;
    addr_size = sizeof(newSocketAddr);
    
    while(1){
        //Accept call creates a new socket for the incoming connection
        newSocket = accept(serverSocket, (struct sockaddr *) &newSocketAddr, &addr_size);
        //for each client request creates a thread and assign the client request to it to process
        //so the main thread can entertain next request

        ptr_threadCommSocketInfo = malloc(sizeof(socketInfo_t));
        ptr_threadCommSocketInfo->sock_fd = newSocket;
        ptr_threadCommSocketInfo->sock_addr = newSocketAddr;
        // ptr_threadCommSocketInfo->sock_addr = malloc(sizeof(struct sockaddr_in));
        // ptr_threadCommSocketInfo->sock_addr->sin_family = newSocketAddr.sin_family;
        // ptr_threadCommSocketInfo->sock_addr->sin_port = newSocketAddr.sin_port;
        // ptr_threadCommSocketInfo->sock_addr->sin_addr.s_addr= newSocketAddr.sin_addr.s_addr;
        // memset(ptr_threadCommSocketInfo->sock_addr->sin_zero, '\0', sizeof ptr_threadCommSocketInfo->sock_addr->sin_zero);

        if( pthread_create(&tid[i], NULL, socketThread, ptr_threadCommSocketInfo) != 0 )
            error("ERROR create thread");
        
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