#include "protocol.h"

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int sendPacket(int socket, const struct Packet *packet){
    int sent;
    int toSend = packet->header.length;

    const uint8_t *ptr = (const uint8_t *) packet;

    while( toSend ) {
        sent = send(socket, ptr, toSend, 0);
        if( sent == 0 )
            return 0;
        if( sent == -1 ) {
            if( errno == EINTR )
                continue;
            return -1;
        }
        toSend -= sent;
        ptr += sent;
    }
    return 1;
}

int recvPacket(int socket, struct Packet *packet){
    bzero(packet, sizeof(packet));
    int received;
    int toReceive = sizeof(struct Header);
    uint8_t *ptr = (uint8_t *) packet;
    
    while(toReceive){
        received = recv(socket, ptr, toReceive, 0);
        if( received == 0 )
            return 0;
        if( received == -1 ) {
            if( errno == EINTR )
                continue;
            return -1;
        }
        toReceive -= received;
        ptr += received;
    }

    toReceive = packet->header.length  - sizeof(struct Header);

    while(toReceive){
        received = recv(socket, ptr, toReceive, 0);
        if( received == 0 )
            return 0;
        if( received == -1 ) {
            if( errno == EINTR )
                continue;
            return -1;
        }
        toReceive -= received;
        ptr += received;
    }

    return 1;
}

void setSignUpPacket(struct Packet *packet, const char *user, const char *pass){
    bzero(packet,sizeof(packet));
    packet->header.command = signup;
    packet->header.length = sizeof(struct Header) + sizeof(struct SignArgs);
    strcpy(packet->payload.signArgs.username, user);
    strcpy(packet->payload.signArgs.password, pass);
}

void setSignInPacket(struct Packet *packet, const char *user, const char *pass){
    bzero(packet,sizeof(packet));
    packet->header.command = signin;
    packet->header.length = sizeof(struct Header) + sizeof(struct SignArgs);
    strcpy(packet->payload.signArgs.username, user);
    strcpy(packet->payload.signArgs.password, pass);
}

void setResponsePacket(struct Packet *packet, enum ResponseValues responseValue){
    bzero(packet,sizeof(packet));
    packet->header.command = response;
    packet->header.length = sizeof(struct Header) + sizeof(struct ResponseArgs);
    packet->payload.responseArgs.value = responseValue;
}

void setSignOutPacket(struct Packet *packet){
    bzero(packet,sizeof(packet));
    packet->header.command = signout;
    packet->header.length = sizeof(struct Header);
}

void setPushPacket(struct Packet *packet){
    bzero(packet,sizeof(packet));
    packet->header.command = push;
    packet->header.length = sizeof(struct Header);
}

void setPullPacket(struct Packet *packet, const char *user){
    bzero(packet,sizeof(packet));
    packet->header.command = pull;
    packet->header.length = sizeof(struct Header) + sizeof(struct PullArgs);
    strcpy(packet->payload.pullArgs.username, user);
}

void setFilePacket(struct Packet *packet, const char *fileName, uint32_t fileSize, bool lastFile){
    bzero(packet,sizeof(packet));
    packet->header.command = file;
    packet->header.length = sizeof(struct Header) + sizeof(struct FileArgs);
    strcpy(packet->payload.fileArgs.name, fileName);
    packet->payload.fileArgs.fileSize = fileSize ;
    packet->payload.fileArgs.lastFile = lastFile ;
}

void setBlockPacket(struct Packet *packet, uint32_t blockLength, const char *blockData){
    bzero(packet,sizeof(packet));
    packet->header.command = block;
    packet->header.length = sizeof(struct Header) + sizeof(struct BlockArgs);
    packet->payload.blockArgs.blockLength = blockLength;
    strcpy(packet->payload.blockArgs.blockData, blockData);
}

enum Command getPacketCommand(struct Packet *packet){
    return packet->header.command;
}

enum ResponseValues getPacketResponseVal(struct Packet *packet){
    return packet->payload.responseArgs.value;
}

bool isSignedIn(clientInfo_t *clientInfo){
    return clientInfo->signedIn;
}

int isRegistered(char *username){
    char userpath[USERLEN + 6];
    strcpy(userpath,USERDIR);
    strcat(userpath, username);
    DIR* dir = opendir(userpath);
    if (dir){
        closedir(dir);
        return 1;
    } else if (ENOENT == errno) {
        return 0;
    }
    return -1;
}

void setClientUser(clientInfo_t *clientInfo, char *username){
    strcpy(clientInfo->username, username);
}

void setClientPass(clientInfo_t *clientInfo, char *password){
    strcpy(clientInfo->password, password);
}

void setClientSignedIn(clientInfo_t *clientInfo, bool signedIn){
    clientInfo->signedIn = signedIn;
}

int isPassCorrect(clientInfo_t *clientInfo){
    int ret;
    char pass[PASSLEN], realPass[PASSLEN], path[PASSLEN + strlen(USERDIR) + strlen(USERFMT)];
    strcpy(pass,clientInfo->password);
    strcpy(path,USERDIR);
    strcat(path, clientInfo->username);
    strcat(path, USERFMT);
    readNthLineFromFile(path, realPass, 0);
    ret = strcmp(pass,realPass);
    if(ret == 0)
        return 1;
    return 0;
}

int readNthLineFromFile(const char *srcPath, char *dest, int nthLine){
    int line = 0;
    FILE * fp;
    char c;
    bzero(dest,sizeof(dest));
    fp = fopen(srcPath, "r");
    while(1) {
        c = fgetc(fp);
        if(line == nthLine){
            if(c == '\n' || feof(fp)){
                fclose(fp);
                return 1;
            } else
                strncat(dest, &c, 1); 
        }
        if(c == '\n'){
            line++;
            continue;
        }

        if (feof(fp)) 
            break; 
    }
    printf("File doesnt have (n+1) lines\n");
    fclose(fp);
    return -1;
}

void getStdInput(char *dest, uint maxLength, clientInfo_t *clientInfo, const char * msg){
    bzero(dest,sizeof(dest));
    if(isSignedIn(clientInfo))
        printf("%s@miniGit: ",clientInfo->username);
    else
        printf("@miniGit: ");
    if(msg!=NULL && strcmp(msg,"")!=0)
        printf("%s> ",msg);
    fgets(dest, maxLength, stdin);
    if ((strlen(dest) > 0) && (dest[strlen (dest) - 1] == '\n'))
        dest[strlen (dest) - 1] = '\0';
}

enum Command typed2enum(const char *typedInCommand){ // Attention: if this is modified, enum Command should also be modified
    if(strcmp(typedInCommand,"signin")==0 || strcmp(typedInCommand,"login")==0)
        return signin;
    else if(strcmp(typedInCommand,"signout")==0 || strcmp(typedInCommand,"logout")==0)
        return signout;
    else if(strcmp(typedInCommand,"signup")==0 || strcmp(typedInCommand,"register")==0)
        return signup;
    else if(strcmp(typedInCommand,"push")==0)
        return push;
    else if(strcmp(typedInCommand,"help")==0)
        return help;
    else if(strcmp(typedInCommand,"exit")==0 || strcmp(typedInCommand,"stop")==0)
        return stop;
    else if(strcmp(typedInCommand,"clear")==0)
        return clearScreen;
    else if(strcmp(typedInCommand,"ls")==0)
        return ls;
    else if(strcmp(typedInCommand,"init")==0)
        return init;
    else if(strcmp(typedInCommand,"commit")==0)
        return commit;
    
    // If its none of the previous commands:
    char *token, *rest, aux[COMMANDLEN];
    strcpy(aux, typedInCommand);
    rest = aux;
    token = strtok_r(rest, " ", &rest);
    
    if(strcmp(token,"pull")==0 || strcmp(token,"clone")==0)
        return pull;
    else if(strcmp(token,"add")==0)
        return add;
    return wrongCommand;
}

int createUser(clientInfo_t *clientInfo){
    char dirPath[USERLEN + strlen(USERDIR)], filePath[USERLEN + strlen(USERDIR) + strlen(USERFMT)];
    FILE *fp;

    strcpy(dirPath, USERDIR);
    strcat(dirPath, clientInfo->username);

    strcpy(filePath, USERDIR);
    strcat(filePath, clientInfo->username);
    strcat(filePath, USERFMT);

    // Create folder
    if(createDir(dirPath) == -1)
        return -1;

    // Create file and insert the password
    fp = fopen(filePath, "w+");
    if (fp != NULL){
        fputs(clientInfo->password, fp);
        fclose(fp);
    }

    return 1;
}

int createDir(const char *dirName){
    struct stat st = {0};
    if (stat(dirName, &st) == -1) {
        mkdir(dirName, 0777);
        return 1;
    }
    return -1;
}

void printFile(const char *filePath){
    FILE *fp;
    char c;
    fp=fopen(filePath,"r");
    while((c=fgetc(fp))!=EOF) {
        printf("%c",c);
    }
    fclose(fp);
}

// The next functions tries to send a directory. If success, returns 1. If error in opening a folder, returns -1. If error in sending a file returns -2
int sendDir(int socket, struct Packet *packet, const char *dirName, int level, bool send ,bool print, int exclude){
    DIR *dir;
    struct dirent *entry;
    int ret;

    if(level == 0){
        mkdir(dirName, 0777); // when client pushes, the client may have deleted by accident the folder, so create it, just in case
        exclude = strlen(dirName) + 1; // plus 1 because of '/'
    }

    // Check if dir can be opened
    if (!(dir = opendir(dirName)))
        return -1;

    // Loop over all files and subdirs of current dir
    while ((entry = readdir(dir)) != NULL) {
        // If entry is a dir
        if (entry->d_type == DT_DIR) {
            char path[PATHLEN];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", dirName, entry->d_name);
            if(print) printf("%*s[%s]\n", (level * 2), "", entry->d_name);
            ret = sendDir(socket, packet, path, level + 1, send, print, exclude); // recursion
            if(ret != 1)    
                return ret;
        } 
        // Else if entry is a file
        else {
            if(print) printf("%*s- %s\n", (level * 2), "", entry->d_name);
            if(send){
                char fileName[PATHLEN];
                snprintf(fileName, sizeof(fileName), "%s/%s", dirName, entry->d_name);
                //Send file
                ret = sendFile(socket, packet, fileName, exclude);
                if(ret == -1)
                    return -2;
            }
        }
    }

    closedir(dir);

    if(level == 0 && send){
        // Send file packet telling that theres no more files to send
        setFilePacket(packet, "", 0, true);
        sendPacket(socket, packet);
    }
    return 1;
}

int sendFile(int socket, struct Packet *packet, const char *fileName, int exclude){
    uint32_t fileSize;
    FILE *fp;
    char buffer[BLOCKLEN + 1];
    int ret;
    
    // Open file and get file size
    fp = fopen(fileName, "r");
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    rewind(fp);

    // Send file packet
    setFilePacket(packet, fileName + exclude, fileSize, false);
    if(sendPacket(socket, packet) == -1)
        return -1;

    // Send block packets
    while(fgets(buffer, BLOCKLEN, fp) != NULL){
        setBlockPacket(packet, strlen(buffer), buffer);
        if(sendPacket(socket, packet) == -1)
            return -1;
    }

    return 1;
}

int recvDir(int socket, struct Packet *packet, const char *rootDir){
    bool lastFile;
    char fileName[FILELEN], filePath[FILELEN];
    uint32_t fileSize;

    remove_directory(rootDir, rootDir);

    do{
        recvPacket(socket, packet);

        if(packet->header.command != file){
            printf("Error: A file type packet was expected\n");
            return -1;
        }

        lastFile = packet->payload.fileArgs.lastFile;
        strcpy(fileName, packet->payload.fileArgs.name);
        fileSize = packet->payload.fileArgs.fileSize;

        if(!lastFile){
            strcpy(filePath, rootDir);
            strcat(filePath, "/");
            strcat(filePath, fileName);
            recvFile(socket, packet, fileSize, filePath);
        }

    } while (!lastFile);
    return 1;
}

int recvFile(int socket, struct Packet *packet, uint32_t fileSize, char *filePath){
    uint32_t toReceive = fileSize;
    FILE *fp;

    // Create folders if needed
    char aux[FILELEN];
    strcpy(aux,filePath);
    char *token, *rest = aux, createDir[FILELEN];
    int i = 0,  nFolders = countOccurrences('/', filePath), ret;
    strcpy(createDir,"");
    while((token = strtok_r(rest, "/", &rest))){
        i++;
        strcat(createDir,token);
        // Create folder if doesnt exists
        ret = mkdir(createDir, 0777);
        if(i==nFolders) break;
        strcat(createDir,"/");
    }

    // Open file
    fp = fopen(filePath,"w+");
    if(fp == NULL){
        printf("Error in opening file\n");
        return -1;
    }
    

    while(toReceive){
        recvPacket(socket, packet);
        
        // Check packet type
        if(packet->header.command != block){
            printf("Error: A block type packet was expected\n");
            return -1;
        }

        fprintf(fp, "%s", packet->payload.blockArgs.blockData);

        toReceive -= packet->payload.blockArgs.blockLength;
    }
    fclose(fp);
    return 1;
}

int countOccurrences(char c, const char *string){
    int i, ret = 0, n = strlen(string);
    for(i = 0; i < n ; i++){
        if(string[i]==c)
            ret++;
    }
    return ret;
}

int remove_directory(const char *path, const char *exclude) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;
    if (d) {
        struct dirent *p;

        r = 0;
        while (!r && (p=readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory(buf, exclude);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r && strcmp(path,exclude)!=0)
        r = rmdir(path);

    return r;
}

void getNthArg(const char * typedInCommand, int n, char * nthArg){
    char *token, *rest, aux[COMMANDLEN];
    strcpy(aux, typedInCommand);
    rest = aux;

    int i = 0;
    while((token = strtok_r(rest, " ", &rest))){
        if(i==n){
            strcpy(nthArg, token);
            return;
        }
        i++;
    }
    bzero(nthArg,sizeof(nthArg));
    
}

void printHelp(void){
printf("------------------------------------------------------------------\n");
printf("------------------------------------------------------------------\n");
printf("                         Use of miniGit\n");
printf("------------------------------------------------------------------\n");
printf("------------------------------------------------------------------\n");
printf("Accepted commands:\n");
printf("username@miniGit: Enter command> help\n");
printf("username@miniGit: Enter command> init\n");
printf("username@miniGit: Enter command> add\n");
printf("username@miniGit: Enter command> commit\n");
printf("username@miniGit: Enter command> checkout HASH\n");
printf("username@miniGit: Enter command> pull USERNAME 'or' clone USERNAME\n");
printf("username@miniGit: Enter command> clear\n");
printf("username@miniGit: Enter command> exit 'OR' stop\n");
printf("\n");
printf("Accepted commands when not signed in (ONLY):\n");
printf("@miniGit: Enter command> login 'or' signin\n");
printf("@miniGit: Enter command> signup 'or' register\n");
printf("\n");
printf("Accepted commands when signed in (ONLY):\n");
printf("username@miniGit: Enter command> pull\n");
printf("username@miniGit: Enter command> push\n");
printf("username@miniGit: Enter command> ls\n");
printf("username@miniGit: Enter command> logout 'or' signout\n");
printf("------------------------------------------------------------------\n");
printf("------------------------------------------------------------------\n");
}