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
    char buffer[BUFFSIZE], typedInCommand[BUFFSIZE], checkPass[PASSLEN];
    struct Packet packet;
    enum ResponseValues responseValue;
    enum Command command;

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

    setClientSignedIn(&clientInfo, false); // client must start as not signed in
    
    while(1){
        // Ask user for command
        getStdInput(typedInCommand, COMMANDLEN, &clientInfo, "Enter command");
        
        command = typed2enum(typedInCommand);

        switch (command){
            case signin:
                // Check if signed in
                if(isSignedIn(&clientInfo)){
                    printf("Already signed in\n");
                    break;
                }

                // Ask user for username and password
                getStdInput(clientInfo.username, USERLEN, &clientInfo, "Enter username");
                getStdInput(clientInfo.password, PASSLEN, &clientInfo, "Enter password");

                // Send packet
                setSignInPacket(&packet, clientInfo.username ,clientInfo.password);
                sendPacket(clientInfo.sock_fd, &packet);

                // Wait for server response
                if(recvPacket(clientInfo.sock_fd, &packet) != 1)
                    error("Error when receiving server response packet\n");
                
                // Check if packet received is a response
                if(getPacketCommand(&packet) != response)
                    error("Error: Packet sent by server should be a response\n");

                // Check which response value the server sent
                responseValue = getPacketResponseVal(&packet);
                if(responseValue != OK){
                    if(responseValue == ErrorA)
                        printf("Error: User not registered\n");
                    else if (responseValue == ErrorB)
                        printf("Error: Wrong username or password\n");
                    else
                        printf("Error: Unkown error\n");
                    break;
                }

                // ---------- OK zone ----------
                setClientSignedIn(&clientInfo, true);
                printf("Signed in successfully\n");
                break;
            
            case signout:
                // Check if signed in
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in\n");
                    break;
                }

                // Send packet
                setSignOutPacket(&packet);
                sendPacket(clientInfo.sock_fd, &packet);

                // Wait for server response
                if(recvPacket(clientInfo.sock_fd, &packet) != 1)
                    error("Error when receiving server response packet\n");

                // Check if packet received is a response
                if(getPacketCommand(&packet) != response)
                    error("Error: Packet sent by server should be a response\n");

                // Check which response value the server sent
                responseValue = getPacketResponseVal(&packet);
                if(responseValue != OK){
                    printf("Error: Unkown error\n");
                    break;
                }

                // ---------- OK zone ----------
                printf("Signed out successfully\n");
                setClientUser(&clientInfo, "");
                setClientPass(&clientInfo, "");
                setClientSignedIn(&clientInfo, false);
                break;
            
            case signup:
                // Check if signed in
                if(isSignedIn(&clientInfo)){
                    printf("You must sign out to sign up with a new account\n");
                    break;
                }

                // Ask user for username and password
                getStdInput(clientInfo.username, USERLEN, &clientInfo, "Enter username");
                getStdInput(clientInfo.password, PASSLEN, &clientInfo, "Enter password");
                getStdInput(checkPass, PASSLEN, &clientInfo, "Repeat password");

                // Check password
                if(strcmp(clientInfo.password,checkPass)!=0){
                    printf("Passwords entered are not the same\n");
                    break;
                }

                // Send packet
                setSignUpPacket(&packet, clientInfo.username ,clientInfo.password);
                sendPacket(clientInfo.sock_fd, &packet);

                // Wait for server response
                if(recvPacket(clientInfo.sock_fd, &packet) != 1)
                    error("Error when receiving server response packet\n");

                // Check if packet received is a response
                if(getPacketCommand(&packet) != response)
                    error("Error: Packet sent by server should be a response\n");

                // Check which response value the server sent
                responseValue = getPacketResponseVal(&packet);
                if(responseValue != OK){
                    if(responseValue == ErrorA)
                        printf("Error: Username already exists, pick another one\n");
                    else
                        printf("Error: Unkown error\n");
                    break;
                }

                // ---------- OK zone ----------
                setClientUser(&clientInfo, "");
                setClientPass(&clientInfo, "");
                printf("You signed up successfully. Now, try to sign in.\n");
                printf("Tip: Remember your password, there is no command to recover it :(\n");
                break;

            case pull:
                break;

            case push:
                break;

            case help:
                printFile(HELPDIR);
                break;
            
            default:
                printf("Unknown command: %s \n",typedInCommand);
                printf("Use command help if needed\n");
                break;
        }
        printf("\n");
    }
    close(clientInfo.sock_fd);
    return 0;
}
