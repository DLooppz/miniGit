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

int isRegistered(clientInfo_t *clientInfo){
    char username[USERLEN + 6];
    strcpy(username,USERDIR);
    strcat(username, clientInfo->username);
    DIR* dir = opendir(username);
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

enum Command typed2enum(char *typedInCommand){ // Attention: if this is modified, enum Command should also be modified
    if(strcmp(typedInCommand,"signin")==0 || strcmp(typedInCommand,"login")==0)
        return signin;
    else if(strcmp(typedInCommand,"signout")==0 || strcmp(typedInCommand,"logout")==0)
        return signout;
    else if(strcmp(typedInCommand,"signup")==0 || strcmp(typedInCommand,"register")==0)
        return signup;
    else if(strcmp(typedInCommand,"pull")==0)
        return pull;
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

void sendPrintDir(int socket, struct Packet *packet, const char *dirName, int level, bool send ,bool print, int exclude){
    DIR *dir;
    struct dirent *entry;

    // Check if dir can be opened
    if (!(dir = opendir(dirName)))
        return;
    
    if(level == 0)
        exclude = strlen(dirName) + 1; // plus 1 because of '/'

    // Loop over all files and subdirs of current dir
    while ((entry = readdir(dir)) != NULL) {
        // If entry is a dir
        if (entry->d_type == DT_DIR) {
            char path[PATHLEN];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", dirName, entry->d_name);
            if(print) printf("%*s[%s]\n", (level * 2), "", entry->d_name);
            sendPrintDir(socket, packet, path, level + 1, send, print, exclude); // recursion
        } 
        // Else if entry is a file
        else {
            if(print) printf("%*s- %s\n", (level * 2), "", entry->d_name);
            if(send){
                char fileName[PATHLEN];
                snprintf(fileName, sizeof(fileName), "%s/%s", dirName, entry->d_name);
                //Send file
                sendFile(socket, packet, fileName, exclude);
            }
        }
    }

    closedir(dir);

    if(level == 0 && send){
        // Send file packet telling that theres no more files to send
        setFilePacket(packet, "", 0, true);
        sendPacket(socket, packet);
    }
}

void sendFile(int socket, struct Packet *packet, const char *fileName, int exclude){
    printf("File to send: %s\n",fileName);
    uint32_t fileSize;
    FILE *fp;
    char buffer[BLOCKLEN + 1];
    
    // Open file and get file size
    fp = fopen(fileName, "r");
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    rewind(fp);
    printf("File size [bytes]: %d\n",fileSize);

    // Send file packet
    setFilePacket(packet, fileName + exclude, fileSize, false);
    sendPacket(socket, packet);

    // Send block packets
    while(fgets(buffer, BLOCKLEN, fp) != NULL){
        setBlockPacket(packet, strlen(buffer), buffer);
        sendPacket(socket, packet);
    }
}

void recvDir(int socket, struct Packet *packet, const char *rootDir){
    bool lastFile;
    char fileName[FILELEN], filePath[FILELEN];
    uint32_t fileSize;

    do{
        recvPacket(socket, packet);

        if(packet->header.command != file){
            printf("Error: A file type packet was expected\n");
            return;
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
}

void recvFile(int socket, struct Packet *packet, uint32_t fileSize, char *filePath){
    uint32_t toReceive = fileSize;
    FILE *fp;
    fp = fopen(filePath,"w+");
    char *token, *rest = filePath;

    if(fp == NULL){
        printf("Error in opening file\n");
        return;
    }

    int nFolders = countOccurrences('/', filePath);

    while((token = strtok_r(rest, "/", &rest)))
        printf("%s\n", token); 

    while(toReceive){
        recvPacket(socket, packet);
        
        // Check packet type
        if(packet->header.command != block){
            printf("Error: A block type packet was expected\n");
            return;
        }

        fprintf(fp, "%s", packet->payload.blockArgs.blockData);

        toReceive -= packet->payload.blockArgs.blockLength;
    }
    fclose(fp);
}

int countOccurrences(char c, const char *string){
    int i, ret = 0, n = strlen(string);
    for(i = 0; i < n ; i++){
        if(string[i]==c)
            ret++;
    }
    return ret;
}