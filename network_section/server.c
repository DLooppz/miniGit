// Run with:
// clear && gcc -pthread server.c lib/protocol.c -lm -o server && ./server

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
#include<stdbool.h>
#include<stdint.h>
#include <dirent.h>
#include"lib/protocol.h"

void * clientThread(void *arg){
    // Variables
    int ret;
    clientInfo_t * ptr_clientInfo;
    struct Packet packet;

    // Thread information structure
    ptr_clientInfo = (clientInfo_t *) arg; 
    
    // Buffer
    char buffer[BUFFSIZE];

    printf("New thread created\n");
    printf("Socket port: %d\n",ptr_clientInfo->sock_addr.sin_port);  

    while(1){
        // Read client packet
        if(recvPacket(ptr_clientInfo->sock_fd, &packet) != 1)
            break;

        // Switch
        switch(packet.header.command){
            case signin:
                // Load username and password ptr_clientInfo, which is the struct that describes the client
                setClientUser(ptr_clientInfo, packet.payload.signArgs.username);
                setClientPass(ptr_clientInfo, packet.payload.signArgs.password);

                // Check if username is registered (did a signup)
                if(isRegistered(ptr_clientInfo) != 1){
                    setResponsePacket(&packet, ErrorA);
                    sendPacket(ptr_clientInfo->sock_fd, &packet);
                    printf("User not registered\n");
                    continue;
                }

                // Check if username/password is correct
                if(isPassCorrect(ptr_clientInfo) != 1){
                    setResponsePacket(&packet, ErrorB);
                    sendPacket(ptr_clientInfo->sock_fd, &packet);
                    printf("Wrong username/password\n");
                    continue;
                }
                
                // ---------- OK zone ----------

                setClientSignedIn(ptr_clientInfo,true); // Set client signedin state to true
                // Send packet
                setResponsePacket(&packet, OK);
                sendPacket(ptr_clientInfo->sock_fd, &packet);
                break;
            
            case signout:
                // ---------- OK zone ---------- (nothing to check, all was checked by client)
                setClientSignedIn(ptr_clientInfo, false);

                // Send packet
                setResponsePacket(&packet, OK);
                sendPacket(ptr_clientInfo->sock_fd, &packet);
                break;

            case signup:
                // Load username and password ptr_clientInfo
                setClientUser(ptr_clientInfo, packet.payload.signArgs.username);
                setClientPass(ptr_clientInfo, packet.payload.signArgs.password);
                
                // Check if username already exists
                if(isRegistered(ptr_clientInfo) == 1){
                    setResponsePacket(&packet, ErrorA);
                    sendPacket(ptr_clientInfo->sock_fd, &packet);
                    printf("Username is already registered\n");
                    continue;
                }

                // ---------- OK zone ----------
                // Create folder
                createUser(ptr_clientInfo);

                // Create file with the password inside

                // Send packet
                setResponsePacket(&packet, OK);
                sendPacket(ptr_clientInfo->sock_fd, &packet);
                break;

            case file:
                break;

            case block:
                break;

            default:
                printf("Command not found\n");
                break;
        }
    }

    // Free memory and close sockets
    printf("Exit thread - The ports being used was %d\n",ptr_clientInfo->sock_addr.sin_port);
    close(ptr_clientInfo->sock_fd);
    free(ptr_clientInfo);

    // End thread
    pthread_exit(NULL);
}

int main(){
    // Variables
    int opt = 1, serverSocket, newSocket, i = 0;
    struct sockaddr_in serverAddr, newSocketAddr;
    socklen_t addr_size;
    clientInfo_t * ptr_clientInfo;
    pthread_t tid[50];

    // Initialization and configuration of socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bzero(serverAddr.sin_zero, sizeof serverAddr.sin_zero);
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    //Listen on the socket, with 50 max connection requests queued 
    printf("Listening\n");
    if(listen(serverSocket,50)==-1)
        error("ERROR in listening server socket");

    addr_size = sizeof(newSocketAddr);
    
    while(1){ // For each client request creates a thread
        // Accept call creates a new socket for the incoming connection
        newSocket = accept(serverSocket, (struct sockaddr *) &newSocketAddr, &addr_size);

        ptr_clientInfo = malloc(sizeof(clientInfo_t));
        ptr_clientInfo->sock_fd = newSocket;
        ptr_clientInfo->sock_addr = newSocketAddr;
        bzero(ptr_clientInfo->username, sizeof(ptr_clientInfo->username));
        bzero(ptr_clientInfo->password, sizeof(ptr_clientInfo->password));
        ptr_clientInfo->signedIn = false;

        //Call thread
        if( pthread_create(&tid[i], NULL, clientThread, ptr_clientInfo) != 0 )
            error("ERROR in creating thread");
        
        // TODO: improve pthread joins
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
