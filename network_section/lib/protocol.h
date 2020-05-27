#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define SERVERPORT 7000
#define BUFFSIZE 512
#define COMMANDLEN 64
#define USERLEN 64
#define PASSLEN 64
#define USERDIR "users/"
#define HELPDIR "lib/help.txt"
#define USERFMT ".txt"

#include<stdlib.h>
#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<errno.h>
#include<stdbool.h>
#include<string.h>
#include<dirent.h>
#include<sys/stat.h>
#include<termios.h>

// Declaration of variables, structs, enums, ...
typedef struct {
    int sock_fd;
    struct sockaddr_in sock_addr;
    char username[USERLEN];
    char password[USERLEN];
    bool signedIn;
} clientInfo_t;

enum Command {  // Attention: if this is modified, function typed2enum should also be modified
    signin,
    signout,
    signup,
    response,
    file,
    block,
    push,       // no packets with this command, but its useful for using a switch in client.c
    pull,       // no packets with this command, but its useful for using a switch in client.c
    help        // no packets with this command, but its useful for using a switch in client.c
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

struct __attribute__((packed)) SignArgs{
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
        struct SignArgs signArgs; // Used for signin or signup
        struct ResponseArgs responseArgs;
        struct FileArgs fileArgs;
        struct BlockArgs blockArgs;
    } payload;
};

// Declaration of functions
void error(const char *msg);

int sendPacket(int socket, const struct Packet *packet);
int recvPacket(int socket, struct Packet *packet);
void setSignUpPacket(struct Packet *packet, const char *user, const char *pass);
void setSignInPacket(struct Packet *packet, const char *user, const char *pass);
void setResponsePacket(struct Packet *packet, enum ResponseValues responseValue);
void setSignOutPacket(struct Packet *packet);
enum Command getPacketCommand(struct Packet *packet);
enum ResponseValues getPacketResponseVal(struct Packet *packet);

void setClientUser(clientInfo_t *clientInfo, char *username);
void setClientPass(clientInfo_t *clientInfo, char *password);
void setClientSignedIn(clientInfo_t *clientInfo, bool signedIn);
bool isSignedIn(clientInfo_t *clientInfo);
int isRegistered(clientInfo_t *clientInfo);
int isPassCorrect(clientInfo_t *clientInfo);

int readNthLineFromFile(const char *srcPath, char *dest, int nthLine);
void getStdInput(char *dest, uint maxLength, clientInfo_t *clientInfo, const char * msg);
int createUser(clientInfo_t *clientInfo);
enum Command typed2enum(char *typedInCommand);
void printFile(const char *filePath);
#endif