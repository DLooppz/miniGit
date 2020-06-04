// Run with:
// clear && gcc miniGit.c lib/miniGitUtils.c -lm -lssl -lcrypto -o miniGit && ./miniGit

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
#include"lib/miniGitUtils.h"

int main(){
    // Clear screen
    clearScreen();

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
        getStdInput(typedInCommand, COMMANDLEN, &clientInfo, "Enter command", false);
        
        command = typed2enum(typedInCommand);

        switch (command){
            case c_signin:
                // Check if signed in
                if(isSignedIn(&clientInfo)){
                    printf("Already signed in\n\n");
                    break;
                }

                // Ask user for username and password
                getStdInput(clientInfo.username, USERLEN, &clientInfo, "Enter username", false);
                getStdInput(clientInfo.password, PASSLEN, &clientInfo, "Enter password", true);

                // Send packet
                setSignInPacket(&packet, clientInfo.username ,clientInfo.password);
                sendPacket(clientInfo.sock_fd, &packet);

                // Wait for server response
                if(recvPacket(clientInfo.sock_fd, &packet) != 1)
                    error("Error when receiving server response packet\n\n");
                
                // Check if packet received is a response
                if(getPacketCommand(&packet) != c_response)
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
            
            case c_signout:
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
                if(getPacketCommand(&packet) != c_response)
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
            
            case c_signup:
                // Check if signed in
                if(isSignedIn(&clientInfo)){
                    printf("You must sign out to sign up with a new account\n\n");
                    break;
                }

                // Ask user for username and password
                getStdInput(clientInfo.username, USERLEN, &clientInfo, "Enter username", false);
                getStdInput(clientInfo.password, PASSLEN, &clientInfo, "Enter password", true);
                getStdInput(checkPass, PASSLEN, &clientInfo, "Repeat password", true);

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
                if(getPacketCommand(&packet) != c_response)
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

            case c_pull:
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
                if(getPacketCommand(&packet) != c_response)
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

                if(strcmp(nThArg, clientInfo.username) != 0){
                    char aux[COMMANDLEN];
                    strcpy(aux, nThArg);
                    strcat(aux, "_CLONED");
                    ret = recvDir(clientInfo.sock_fd, &packet, aux);
                } else
                    ret = recvDir(clientInfo.sock_fd, &packet, nThArg);

                // Receive data
                if(ret == -1){
                    printf("Error when receiving data from server.");
                    stopClient = true;
                    break;
                }

                // ---------- OK zone ----------
                if(strcmp(nThArg, clientInfo.username) == 0){
                    printf("Pulled successfully (Data saved in ./%s)\n",nThArg);
                    printf("You can use the command ls to check your data.\n");
                }
                else if(clientInfo.signedIn == true){
                    printf("Cloned successfully (Data saved in ./%s_CLONED)\n",nThArg);
                    printf("You have pulled a repo that belongs to other user. Your working directory remains exactly the same.\n");
                } else
                    printf("Cloned successfully (Data saved in ./%s_CLONED)\n",nThArg);
                printf("\n");
                break;

            case c_push:
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
                if(getPacketCommand(&packet) != c_response)
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

            case c_help:
                printHelp();
                printf("\n");
                break;
            
            case c_stop:
                stopClient = true;
                printf("Exiting\n");
                printf("\n");
                break;
            
            case c_clearScreen:
                clearScreen();
                break;
            
            case c_ls:
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
            
            case c_init:
                // Check if signed in
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in\n\n");
                    break;
                }

                // OK zone
                init(&clientInfo);
                printf("\n");
                break;
            
            case c_commit:
                // Check if signed in
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in\n\n");
                    break;
                }

                // Get message
                char msg[MSGLEN];
                getMsg(typedInCommand, msg, MSGLEN);
                
                // Check if message is not null
                if(msg[0] == '\0'){
                    printf("You must enter a message in a commit.\n");
                    printf("Usage: commit YOUR MESSAGE HERE\n\n");
                    break;
                }    
                
                // Check if USERNAME/.miniGit/index exists
                char indexPath[SMALLPATHLEN];
                strcpy(indexPath, clientInfo.username);
                strcat(indexPath, INDEX_PATH);
                if( simpleCheckFileExistance(indexPath) == -1 ) {
                    printf("You must use the command `add` before `commit` in order to add files to the staging area\n");
                    printf("In case you did an `add` its possible that you did it without any files in your working directory.\n");
                    printf("Try with adding some files. Then `add` and then `commit WITH YOUR COMMIT MESSAGE`\n\n");
                    break;
                }

                // OK zone (internally checks if index is not empty)
                commit(msg, &clientInfo);
                printf("\n");
                break;

            case c_add:
                // Check if signed in
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in\n\n");
                    break;
                }

                // Check if init was made
                char miniGitPath[SMALLPATHLEN];
                strcpy(miniGitPath, clientInfo.username);
                strcat(miniGitPath, "/.miniGit");
                if( simpleCheckFileExistance(miniGitPath) == -1 ) {
                    printf("You need to use the command `init` before `add`\n\n");
                    break;
                }

                // Check if user entered `add` or `add .`
                getNthArg(typedInCommand, 1, nThArg);
                if(nThArg[0]!='\0' && strcmp(nThArg, ".")!=0){
                    printf("Up to now, miniGit only allows you to add all your files\n");
                    printf("So, only two possible commands are permitted using `add`. These commands are: `add` or `add .`\n\n");
                    break;
                }

                // OK zone 
                add(&clientInfo);
                printf("\n");
                break;
            
            case c_checkout:
                // Check if signed in
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in\n\n");
                    break;
                }

                // Check if version is not null
                char version[MSGLEN];
                getMsg(typedInCommand, version, MSGLEN);
                if(version[0] == '\0'){
                    printf("You must enter a version to check (HASH or BRANCH-NAME) in a checkout.\n");
                    printf("Usage: checkout VERSION\n\n");
                    break;
                }    

                // Check that only one argument was given
                getNthArg(typedInCommand, 2, nThArg);
                if (nThArg[0] != '\0'){
                    printf("Usage: checkout VERSION\n\n");
                    break;
                }

                // OK zone
                char returnedMessage[PATHS_MAX_SIZE];
                if (checkout(version,&clientInfo,returnedMessage) == 0)
                    printf("Error! %s\n",returnedMessage);
                else
                    printf("Success! %s\n",returnedMessage);
                break;
            
            case c_cat_file: ; //This ; is an empty statement for compiler "just stop being so palurdo and let me work!"

                // Declare
                char hashToCat[128];
                char typeOfCat[128];
                char messageRet[PATHS_MAX_SIZE];

                // Check that only two argument were given
                getNthArg(typedInCommand, 1, hashToCat);
                getNthArg(typedInCommand, 2, typeOfCat);
                getNthArg(typedInCommand, 3, nThArg);
                if (nThArg[0] != '\0' || hashToCat[0] == '\0' || typeOfCat[0] == '\0'){
                    printf("Usage: cat-file HASH [TYPE]\n\n");
                    printf("[TYPE]: -p (content) or -t (header)\n");
                    break;
                }

                // Cat file or error message
                if (cat_file(hashToCat,typeOfCat,&clientInfo,messageRet) == 0)
                    printf("%s",messageRet);
                break;

            case c_log: 
                // Check if signed in
                if(!isSignedIn(&clientInfo)){
                    printf("You are not signed in\n\n");
                    break;
                }

                // Check that no argument was given
                getNthArg(typedInCommand, 1, nThArg);
                if (nThArg[0] != '\0'){
                    printf("No arguments needed\nUsage: log\n\n");
                    break;
                } 
                printf("\n");
                logHist(&clientInfo);
                break;
            
            case c_enter:
                break;

            case c_wrongCommand:
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
