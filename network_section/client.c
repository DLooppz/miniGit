// Run with:
// clear && gcc client.c lib/protocol.c -o client && ./client

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
#include <stdbool.h>
#include"lib/protocol.h"

int main(){
    // Variables
    int opt = 1, ret;
    clientInfo_t clientInfo;
    socklen_t addrSize;
    char buffer[BUFFSIZE], typedInCommand[BUFFSIZE];
    struct Packet packet;
    enum ResponseValues responseValue;

    // Initialization and configuration of socket
    bzero(&clientInfo, sizeof(clientInfo));
    clientInfo.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(clientInfo.sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));
    clientInfo.sock_addr.sin_family = AF_INET;
    clientInfo.sock_addr.sin_port = htons(SERVERPORT);
    clientInfo.sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Connect the socket to the server using the address
    addrSize = sizeof(clientInfo.sock_addr);
    if(connect(clientInfo.sock_fd, (struct sockaddr *) &clientInfo.sock_addr, addrSize) == -1)
        error("ERROR in connecting to server");

    setClientLoggedIn(&clientInfo, false); // client must start as not logged in
    
    while(1){
        // Ask user for command
        printf("Enter a command and then hit enter.\n");
        scanf("%s", typedInCommand); //fgets

        if(strcmp(typedInCommand,"login") == 0){
            if(isLoggedIn(&clientInfo)){
                printf("Already logged in\n");
                continue;
            }

            printf("Enter username.\n");
            scanf("%s", clientInfo.username);
            printf("Enter password.\n");
            scanf("%s", clientInfo.password);

            setLoginPacket(&packet, clientInfo.username ,clientInfo.password);
            sendPacket(clientInfo.sock_fd, &packet);

            // Wait for server response
            if(recvPacket(clientInfo.sock_fd, &packet) != 1){
                printf("Error when receiving server response packet\n");
                break;
            }
            
            if(getPacketCommand(&packet) != response){
                printf("Error: Packet sent by server should be a response\n");
                break;
            }

            responseValue = getPacketResponseVal(&packet);
            if(responseValue != OK){
                if(responseValue == ErrorA)
                    printf("Error: User not registered\n");
                else if (responseValue == ErrorB)
                    printf("Error: Wrong username or password");
                else
                    printf("Error: Unkown error");
                continue;
            }

            // OK zone
            setClientLoggedIn(&clientInfo, true);
        }
    }
    close(clientInfo.sock_fd);
    return 0;
}
