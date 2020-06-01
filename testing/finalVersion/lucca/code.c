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

#define HASHHH "2d57f070e520e01b9670f9cbc68eb2cca720d6d1"


int main(int argc, char *argv[]){

    char pathFound[PATHS_MAX_SIZE];
    int findStatus = 0;
    char* content;

    // findObject("./.miniGit/objects",HASHHH,pathFound,&findStatus);
    // if (findStatus == 1)
    // {
    //     printf("pathfound: %s\n",pathFound);
    //     content = getContentFromObject(pathFound,"-p");
    //     printf("Content:\n%s\n",content);
    // }
    // else
    // {
    //     printf("Path not found\n");

    // }
    // free(content);
    clientInfo_t clientInfo;
    strcpy(clientInfo.username,"lucca");
    if (buildUpDir("./lucca",HASHHH,&clientInfo)==1)
        printf("Succes!\n");
    else
        printf("Fail, try again!\n");

    return 0;
}

// hola