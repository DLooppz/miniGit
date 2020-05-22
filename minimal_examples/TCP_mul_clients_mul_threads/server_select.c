#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>  

#define SIZE 1024

char client_message[SIZE + 1];
char buffer[SIZE + 1];
const int max_clients = 100; // On the final version, max_clients will be higher than max_threads
const int max_threads = 10;
//int client_socket[max_clients];
//struct sockaddr_in * clients_address[max_clients];
// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int socket_fd;
    struct sockaddr_in clientAddr;
    int status;
} threadSocketInfo;

threadSocketInfo myClients[max_clients];

void * socketThreadTest(void *arg){
    int n_bytes;
    threadSocketInfo * ptr_threadSocketInfo;
    ptr_threadSocketInfo = (threadSocketInfo *) arg;
    int newSocket = ptr_threadSocketInfo->socket_fd;
    char *client_message = (char *) malloc(SIZE * sizeof(char));
    char *buffer = (char *) malloc(SIZE * sizeof(char));
    
    printf("Init socketThread - The port being used is  %d \n",ptr_threadSocketInfo->clientAddr->sin_port);
    
    while(1){
        n_bytes = recv(newSocket , client_message , 1024 , 0);
        printf("%s\n",client_message);

        if(strcmp(client_message,"stop") == 0){
            // TO DO
            free(ptr_threadSocketInfo->clientAddr);
            free(ptr_threadSocketInfo);
            for(int i = 0; i < max_clients; i++){
                if(client_socket[i]==newSocket){
                    client_socket[i] = 0;
                    free(clients_address[i]);
                    break;
                }
            }
            close(newSocket);
            break;
        }

        if(strcmp(client_message,"getStatus") == 0){
            // TO DO
            break;
        }

        if(strcmp(client_message,"push") == 0){
            // TO DO
            if(client has pushed all files)
                break;
        }

        strcpy(buffer,"Hello Client : ");
        strcat(buffer,client_message);
        strcat(buffer,"\0");
        send(newSocket,buffer,strlen(buffer),0);

        bzero(buffer, sizeof(buffer));
        bzero(client_message, sizeof(client_message));
    }

    // printf("Exit socketThread - The port being used was %d \n",ptr_threadSocketInfo->clientAddr->sin_port);
    // free(ptr_threadSocketInfo->clientAddr);
    // free(ptr_threadSocketInfo);
    free(buffer);
    free(client_message);

    pthread_exit(NULL);
}

// void * socketThread(void *arg){
//     int n_bytes;
//     threadSocketInfo * ptr_threadSocketInfo;
//     ptr_threadSocketInfo = (threadSocketInfo *) arg;
//     int newSocket = ptr_threadSocketInfo->socket_fd;
//     char *client_message = (char *) malloc(SIZE * sizeof(char));
//     char *buffer = (char *) malloc(SIZE * sizeof(char));
    
//     printf("Init socketThread - The port being used is  %d \n",ptr_threadSocketInfo->clientAddr->sin_port);
    
//     while(1){
//         n_bytes = recv(newSocket , client_message , 1024 , 0);

//         printf("%s\n",client_message);
//         if(strcmp(client_message,"stop") == 0){
//             printf("here\n");
//             break;
//         }

//         strcpy(buffer,"Hello Client : ");
//         strcat(buffer,client_message);
//         strcat(buffer,"\0");
//         send(newSocket,buffer,strlen(buffer),0);

//         bzero(buffer, sizeof(buffer));
//         bzero(client_message, sizeof(client_message));
//     }

//     printf("Exit socketThread - The port being used was %d \n",ptr_threadSocketInfo->clientAddr->sin_port);
//     close(newSocket);
//     free(ptr_threadSocketInfo->clientAddr);
//     free(ptr_threadSocketInfo);
//     free(buffer);
//     free(client_message);
//     pthread_exit(NULL);
// }

int main(){
    int opt = 1;
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addr_size;

    // Create the socket. 
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));

    // Configure settings of the server address struct
    // Address family = Internet 
    serverAddr.sin_family = AF_INET;

    // Set port number, using htons function to use proper byte order 
    serverAddr.sin_port = htons(7799);

    // Set IP address to localhost 
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Set all bits of the padding field to 0 
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    // Bind the address struct to the socket 
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    // Listen on the socket, with 50 max connection requests queued 
    if(listen(serverSocket, 50)==0)
        printf("Listening\n");
    else
        printf("Error\n");

    threadSocketInfo * ptr_threadSocketInfo;
    int i = 0;
    bzero(clients_address, sizeof(clients_address));
    pthread_t tid[max_threads];
    bzero(client_socket, sizeof(client_socket));
    int max_sfd, sfd, activity, valread;
    fd_set readfds; // set of socket file descriptors
    int clientAddrSize = sizeof(clientAddr);
    char *welcomeMsg = "You are connected to miniGit\n";

    while(1){
        printf("----------------------------\n");
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(serverSocket, &readfds);   
        max_sfd = serverSocket;   

        // Add child sockets to set  
        for ( i = 0 ; i < max_clients ; i++){
            // Socket descriptor  
            sfd = client_socket[i];   
                 
            // If valid socket descriptor then add to read list  
            if(sfd > 0 && myClients[i].status==1)  
                FD_SET( sfd , &readfds);   
                 
            // Highest file descriptor number, need it for the select function  
            if(sfd > max_sfd)   
                max_sfd = sfd;   
        }   

        // Wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        activity = select( max_sfd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR)){
            printf("select error");   
        }   
             
        // If something happened on the master socket, then its an incoming connection  
        if (FD_ISSET(serverSocket, &readfds)){
            if ((newSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, (socklen_t*)&clientAddrSize))<0){
                perror("accept");   
                exit(EXIT_FAILURE);   
            }
            
            // Inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d  \n" , newSocket , inet_ntoa(clientAddr.sin_addr) , ntohs (clientAddr.sin_port));
           
            // Send a greeting message to new connection
            if( send(newSocket, welcomeMsg, strlen(welcomeMsg), 0) != strlen(welcomeMsg)){   
                perror("send");   
            }   
            puts("Welcome message sent successfully");   
                 
            // Add new socket to array of sockets  
            for (i = 0; i < max_clients; i++){
                // If position is empty  
                if( myClients[i].socket_fd == 0 ){
                    printf("Adding to list of sockets as %d\n" , i);   
                    myClients[i].socket_fd = newSocket;
                    myClients[i].clientAddr = clientAddr;
                    break;
                }
            }
        }
             
        // Lets check if there is an IO operation on some other socket (could be more than one, depending on how the select function updated the set of socket file descriptors "readfds") 
        for (i = 0; i < max_clients; i++){
            sfd = client_socket[i];
            
            if (FD_ISSET( sfd , &readfds)){

                // ptr_threadSocketInfo = malloc(sizeof(threadSocketInfo));
                // ptr_threadSocketInfo->socket_fd = sfd;
                // ptr_threadSocketInfo->clientAddr = clients_address[i];
                if(condition){
                    if( pthread_create(&tid[i], NULL, socketThreadTest, &myClients[i]) != 0 )
                        printf("Failed to create thread\n");
                    else
                        printf("Calling thread with sfd = %d\n",sfd);
                }
                
                // // Check if it was for closing , and also read the incoming message
                // printf("Socket fd %d is set\n", sfd);
                // if ((valread = read( sfd , buffer, SIZE)) == 0){
                //     // Somebody disconnected , get his details and print  
                //     getpeername(sfd, (struct sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize);  
                //     printf("Host disconnected, ip %s, port %d \n",  inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));   
                         
                //     // Close the socket and mark as 0 in list for reuse  
                //     close(sfd);   
                //     client_socket[i] = 0;
                //     free(clients_address[i]);
                // }
                     
                // //Echo back the message that came in  
                // else {
                //     printf("Socket fd %d sent a message: %s\n", sfd, buffer);
                //     // Set the string terminating NULL byte on the end of the data read  
                //     buffer[valread] = '\0';
                //     send(sfd, buffer, strlen(buffer), 0);
                //     bzero(buffer, sizeof(buffer));
                //     if( pthread_create(&tid[i], NULL, socketThreadTest, &sfd) != 0 )
                //         printf("Failed to create thread\n");
                //     else
                //         printf("Calling thread with sfd = %d\n",sfd);
                // }
            }   
        }   
    }   

    // while(1){
    //     //Accept call creates a new socket for the incoming connection
    //     newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
    //     //for each client request creates a thread and assign the client request to it to process
    //     //so the main thread can entertain next request

    //     ptr_threadSocketInfo = malloc(sizeof(threadSocketInfo));
    //     ptr_threadSocketInfo->socket_fd = newSocket;
    //     ptr_threadSocketInfo->clientAddr = malloc(sizeof(struct sockaddr_in));
    //     ptr_threadSocketInfo->clientAddr->sin_family = serverStorage.sin_family;
    //     ptr_threadSocketInfo->clientAddr->sin_port = serverStorage.sin_port;
    //     ptr_threadSocketInfo->clientAddr->sin_addr.s_addr= serverStorage.sin_addr.s_addr;
    //     memset(ptr_threadSocketInfo->clientAddr->sin_zero, '\0', sizeof ptr_threadSocketInfo->clientAddr->sin_zero);

    //     if( pthread_create(&tid[i], NULL, socketThread, ptr_threadSocketInfo) != 0 )
    //         printf("Failed to create thread\n");
        
    //     if( i >= 40){
    //         i = 0;
    //         while(i < 50){
    //             pthread_join(tid[i++],NULL);
    //         }
    //         i = 0;
    //     }
    //     i++;
        
    // }
    return 0;
}