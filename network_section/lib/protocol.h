#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define SERVERPORT 7000
#define BUFFSIZE 512
#define USERLEN 64
#define PASSLEN 64
#define USERDIR "users/"

#include<stdlib.h>
#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<errno.h>
#include<stdbool.h>
#include<string.h>
#include <dirent.h>

// Declaration of variables, structs, enums, ...
typedef struct {
    int sock_fd;
    struct sockaddr_in sock_addr;
    char username[USERLEN];
    char password[USERLEN];
    bool loggedIn;
} clientInfo_t;

enum Command {
    login,
    logout,
    response,
    file
};

enum ResponseValues {
    OK,
    ErrorA,
    ErrorB
};

struct __attribute__((packed)) Header{
    uint16_t command;   // <<<< enum Commands
    uint16_t length;    // Lenght in bytes of complete packet
};

struct __attribute__((packed)) LoginArgs{
    char username[USERLEN];
    char password[PASSLEN];
};

struct __attribute__((packed)) ResponseArgs{
    uint16_t value;   // <<<<<< enum ResponseValues
};

struct __attribute__((packed)) FileArgs{
    char name[128];
    uint32_t totalLength;
};

struct __attribute__((packed)) BlockArgs{
    uint32_t blockLength;
    char block[1024];
};

struct __attribute__((packed)) Packet{
    struct Header header;
    union Payload {
        struct LoginArgs loginArgs;
        struct ResponseArgs responseArgs;
        struct FileArgs fileArgs;
        struct BlockArgs blockArgs;
    } payload;
};

// Declaration of functions
void error(const char *msg);

int sendPacket(int socket, const struct Packet *packet);
int recvPacket(int socket, struct Packet *packet);
void setLoginPacket(struct Packet *packet, const char *user, const char *pass);
void setResponsePacket(struct Packet *packet, enum ResponseValues responseValue);
enum Command getPacketCommand(struct Packet *packet);
enum ResponseValues getPacketResponseVal(struct Packet *packet);

void setClientUser(clientInfo_t *clientInfo, char *username);
void setClientPass(clientInfo_t *clientInfo, char *password);
void setClientLoggedIn(clientInfo_t *clientInfo, bool loggedIn);
bool isLoggedIn(clientInfo_t *clientInfo);
int isRegistered(clientInfo_t *clientInfo);
int isPassCorrect(clientInfo_t *clientInfo);

int readNthLineFromFile(const char *srcPath, char *dest, int nthLine);

#endif