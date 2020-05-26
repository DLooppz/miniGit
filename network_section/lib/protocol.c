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

void setLoginPacket(struct Packet *packet, const char *user, const char *pass){
    bzero(packet,sizeof(packet));
    packet->header.command = login;
    packet->header.length = sizeof(struct Header) + sizeof(struct LoginArgs);
    strcpy(packet->payload.loginArgs.username, user);
    strcpy(packet->payload.loginArgs.password, pass);
}

void setResponsePacket(struct Packet *packet, enum ResponseValues responseValue){
    bzero(packet,sizeof(packet));
    packet->header.command = response;
    packet->header.length = sizeof(struct Header) + sizeof(struct ResponseArgs);
    packet->payload.responseArgs.value = responseValue;
}

enum Command getPacketCommand(struct Packet *packet){
    return packet->header.command;
}

enum ResponseValues getPacketResponseVal(struct Packet *packet){
    return packet->payload.responseArgs.value;
}

bool isLoggedIn(clientInfo_t *clientInfo){
    return clientInfo->loggedIn;
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

void setClientLoggedIn(clientInfo_t *clientInfo, bool loggedIn){
    clientInfo->loggedIn = loggedIn;
}

int isPassCorrect(clientInfo_t *clientInfo){
    int ret;
    char pass[PASSLEN], realPass[PASSLEN], path[PASSLEN + strlen(USERDIR) + strlen(".txt")];
    strcpy(pass,clientInfo->password);
    strcpy(path,USERDIR);
    strcat(path, clientInfo->username);
    strcat(path, ".txt");
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
            if(c == '\n'){
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
    printf("Theres some error");
    fclose(fp);
    return -1;
}