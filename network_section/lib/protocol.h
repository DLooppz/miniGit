#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define SERVERPORT 7000
#define BLOCKLEN 1024
#define FILELEN 128
#define COMMANDLEN 128
#define USERLEN 64
#define PASSLEN 64
#define PATHLEN 1024
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
#include <unistd.h>

// Declaration of variables, structs, enums, ...
typedef struct {
    int sock_fd;
    struct sockaddr_in sock_addr;
    char username[USERLEN];
    char password[USERLEN];
    bool signedIn;
} clientInfo_t;

enum Command {      // Attention: if this is modified, function typed2enum should also be modified
    signin,
    signout,
    signup,
    response,
    file,
    block,
    push,
    pull,
    help,           // no packets with this command, but its useful for using a switch in client.c
    stop,           // no packets with this command, but its useful for using a switch in client.c
    clearScreen,    // no packets with this command, but its useful for using a switch in client.c
    ls              // no packets with this command, but its useful for using a switch in client.c
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
    char name[FILELEN];
    uint32_t fileSize;
    bool lastFile;
};

struct __attribute__((packed)) BlockArgs{
    uint32_t blockLength;
    char blockData[BLOCKLEN];
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
void setPushPacket(struct Packet *packet);
void setFilePacket(struct Packet *packet, const char *fileName, uint32_t fileSize, bool lastFile);
void setBlockPacket(struct Packet *packet, uint32_t blockLength, const char *blockData);
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
int createDir(const char *dirName);
enum Command typed2enum(char *typedInCommand);
void printFile(const char *filePath);
void sendPrintDir(int socket, struct Packet *packet, const char *dirName, int level, bool send ,bool print, int exclude);
void sendFile(int socket, struct Packet *packet, const char *filename, int exclude);
void recvDir(int socket, struct Packet *packet, const char *rootDir);
void recvFile(int socket, struct Packet *packet, uint32_t fileSize, char *filePath);
int countOccurrences(char c, const char *string);
int remove_directory(const char *path, const char *exclude);
#endif