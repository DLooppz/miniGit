#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils.h"

#define USERNAME1 "Lucca"
#define USERNAME2 "Matias"

int main(int argc, char *argv[]){

    commit(argc,argv,USERNAME1);
    return 0;
}