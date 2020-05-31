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
    char typedInCommand[COMMANDLEN], checkPass[PASSLEN], nThArg[COMMANDLEN];
    struct Packet packet;
    enum ResponseValues responseValue;
    enum Command command;
    bool stopClient = false;


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

    printf("Welcome to miniGit. To see the available commands, type `help` and hit enter\n");
    
    while(1){
        // Ask user for command
        getStdInput(typedInCommand, COMMANDLEN, &clientInfo, "Enter command");
        
        command = typed2enum(typedInCommand);

        switch (command){
            case signin:
                // Check if signed in
                if(isSignedIn(&clientInfo)){
                    printf("Already signed in\n\n");
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
                    error("Error when receiving server response packet\n\n");
                
                // Check if packet received is a response
                if(getPacketCommand(&packet) != response)
                    error("Error: Packet sent by server should be a response\n\n");

                // Check which response value the server sent
                responseValue = getPacketResponseVal(&packet);
                if(responseValue != OK){
                    if(responseValue == ErrorA)
                        printf("Error: User not registered\n\n");
                    else if (responseValue == ErrorB)
                        printf("Error: Wrong username or password\n\n");
                    else
                        printf("Error: Unkown error\n\n");
                    break;
                }

                // ---------- OK zone ----------
                mkdir(clientInfo.username, 0777);
                setClientSignedIn(&clientInfo, true);
                printf("Signed in successfully\n");
                printf("\n");
                break;
            
            case signout:
                // Check if signed in
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in\n\n");
                    break;
                }

                // Send packet
                setSignOutPacket(&packet);
                sendPacket(clientInfo.sock_fd, &packet);

                // Wait for server response
                if(recvPacket(clientInfo.sock_fd, &packet) != 1)
                    error("Error when receiving server response packet\n\n");

                // Check if packet received is a response
                if(getPacketCommand(&packet) != response)
                    error("Error: Packet sent by server should be a response\n\n");

                // Check which response value the server sent
                responseValue = getPacketResponseVal(&packet);
                if(responseValue != OK){
                    printf("Error: Unkown error\n\n");
                    break;
                }

                // ---------- OK zone ----------
                printf("Signed out successfully\n");
                setClientUser(&clientInfo, "");
                setClientPass(&clientInfo, "");
                setClientSignedIn(&clientInfo, false);
                printf("\n");
                break;
            
            case signup:
                // Check if signed in
                if(isSignedIn(&clientInfo)){
                    printf("You must sign out to sign up with a new account\n\n");
                    break;
                }

                // Ask user for username and password
                getStdInput(clientInfo.username, USERLEN, &clientInfo, "Enter username");
                getStdInput(clientInfo.password, PASSLEN, &clientInfo, "Enter password");
                getStdInput(checkPass, PASSLEN, &clientInfo, "Repeat password");

                // Check password
                if(strcmp(clientInfo.password,checkPass)!=0){
                    printf("Passwords entered are not the same\n\n");
                    break;
                }

                // Send packet
                setSignUpPacket(&packet, clientInfo.username ,clientInfo.password);
                sendPacket(clientInfo.sock_fd, &packet);

                // Wait for server response
                if(recvPacket(clientInfo.sock_fd, &packet) != 1)
                    error("Error when receiving server response packet\n\n");

                // Check if packet received is a response
                if(getPacketCommand(&packet) != response)
                    error("Error: Packet sent by server should be a response\n\n");

                // Check which response value the server sent
                responseValue = getPacketResponseVal(&packet);
                if(responseValue != OK){
                    if(responseValue == ErrorA)
                        printf("Error: Username already exists, pick another one\n\n");
                    else
                        printf("Error: Unkown error\n\n");
                    break;
                }

                // ---------- OK zone ----------
                setClientUser(&clientInfo, "");
                setClientPass(&clientInfo, "");
                printf("You signed up successfully. Now, try to sign in.\n");
                printf("Remember your password, there is no command to recover it :(\n");
                printf("\n");
                break;

            case pull:
                // Check if user entered "pull" or "pull username". In this example, username would be the second argument
                getNthArg(typedInCommand, 1, nThArg);
                if(nThArg[0]=='\0'){
                    // Check if signed in
                    if(!isSignedIn(&clientInfo)){
                        printf("You must sign in to pull your directory from the server\n\n");
                        break;
                    }

                    if(strlen(nThArg)>USERLEN){
                        printf("Username too long\n\n");
                        break;
                    }

                    strcpy(nThArg, clientInfo.username);
                }

                // Send pull packet
                setPullPacket(&packet, nThArg);
                sendPacket(clientInfo.sock_fd, &packet);
                
                // Wait for server response
                if(recvPacket(clientInfo.sock_fd, &packet) != 1)
                    error("Error when receiving server response packet\n\n");

                // Check if packet received is a response
                if(getPacketCommand(&packet) != response)
                    error("Error: Packet sent by server should be a response\n\n");

                // Check which response value the server sent
                responseValue = getPacketResponseVal(&packet);
                if(responseValue != OK){
                    if(responseValue == ErrorA)
                        printf("Error: Username doesnt exist\n\n");
                    else
                        printf("Error: Unkown error\n\n");
                    break;
                }

                // Just in case, create folder locally (it should exist)
                createDir(nThArg);

                // Receive data
                ret = recvDir(clientInfo.sock_fd, &packet, nThArg);
                if(ret == -1){
                    printf("Error when receiving data from server.");
                    stopClient = true;
                    break;
                }

                // ---------- OK zone ----------
                printf("Pulled successfully (Data saved in ./%s)\n",nThArg);
                if(strcmp(nThArg, clientInfo.username) == 0)
                    printf("You can use the command ls to check your data.");
                else if(strcmp(nThArg, clientInfo.username) != 0 && clientInfo.signedIn == true)
                    printf("You have pulled a repo that belongs to other user. Your working directory remains exactly the same.\n");
                printf("\n");
                break;

            case push:
                // Check if signed in
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in\n\n");
                    break;
                }

                // Send push packet (this packet just tells the server to be ready for a push)
                setPushPacket(&packet);
                sendPacket(clientInfo.sock_fd, &packet);

                // Send working directory
                ret = sendDir(clientInfo.sock_fd, &packet, clientInfo.username, 0, true, false, 0);
                if(ret != 1){
                    if(ret == -1)
                        printf("Error when trying to open a folder\n");
                    else if(ret == -2)
                        printf("Error when trying to send a file\n");
                    stopClient = true;
                    break;
                }

                // Wait for server response
                if(recvPacket(clientInfo.sock_fd, &packet) != 1)
                    error("Error when receiving server response packet\n\n");

                // Check if packet received is a response
                if(getPacketCommand(&packet) != response)
                    error("Error: Packet sent by server should be a response\n\n");

                // Check which response value the server sent
                responseValue = getPacketResponseVal(&packet);
                if(responseValue != OK){
                    if(responseValue == ErrorA)
                        printf("Error: Error in server. The server was expecting a file type packet.\n\n");
                    else
                        printf("Error: Unkown error\n\n");
                    stopClient = true;
                    break;
                }

                // ---------- OK zone ----------
                printf("Pushed successfully\n");
                printf("\n");
                break;

            case help:
                printHelp();
                printf("\n");
                break;
            
            case stop:
                stopClient = true;
                printf("Exiting\n");
                printf("\n");
                break;
            
            case clearScreen:
                printf("\e[1;1H\e[2J");
                break;
            
            case ls:
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in, so you dont have a working directory.\n\n");
                    break;
                }

                // ---------- OK zone ----------
                printf("Your working directory is: ./%s\nContent of your working directory:\n",clientInfo.username);
                printf("-----------------------------------------\n");
                sendDir(0, NULL, clientInfo.username, 0, false, true, 0);
                printf("-----------------------------------------\n");
                printf("If you make a push, the files and dirs shown will be sent to server\n");
                printf("If you make a pull, the files and dirs shown will be MODIFIED or DELETED\n");
                printf("\n");
                break;
            
            case init:
                printf("Init...\n");
                printf("\n");
                break;
            
            case commit:
                printf("Commit...\n");
                printf("\n");
                break;

            case add:
                printf("Add...\n");
                printf("\n");
                break;

            case wrongCommand:
                printf("Unknown command: %s \n",typedInCommand);
                printf("Use command help if needed\n");
                printf("\n");
                break;
        }

        if(stopClient) break;
    }
    close(clientInfo.sock_fd);
    return 0;
}
