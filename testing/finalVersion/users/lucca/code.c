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

#define PATH "/home/dloopz/Documentos/Materias/LaboratorioDeRedes/Programming/miniGit/testing/finalVersion/lucca/Cosas"
#define HASHHH "3ae51adcc00e529017b1b377c3e2e436ad5e9e57"


int main(int argc, char *argv[]){

    char pathFound[PATHS_MAX_SIZE];
    int findStatus = 0;
    char* content;

    findObject("./.miniGit/objects",HASHHH,pathFound,&findStatus);
    if (findStatus == 1)
    {
        printf("pathfound: %s\n",pathFound);
        content = getContentFromObject(pathFound,"-f");
        printf("Content:\n%s\n",content);
    }
    else
    {
        printf("Path not found\n");

    }
    free(content);

    return 0;
}

// hola