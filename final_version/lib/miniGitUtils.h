#ifndef MINIGITUTILS_H_
#define MINIGITUTILS_H

#define SERVERPORT 7000
#define BLOCKLEN 1024
#define FILELEN 128
#define COMMANDLEN 128
#define USERLEN 64
#define PASSLEN 64
#define SMALLPATHLEN 128
#define PATHLEN 1024
#define USERDIR "users/"
#define HELPDIR "lib/help.txt"
#define USERFMT ".txt"
#define TREE_MAX_SIZE 14000
#define PATHS_MAX_SIZE 512
#define TREE_LINE_MAX_SIZE 512
#define MSGLEN 128
#define MINIGIT_PATH "/.miniGit/"
#define INDEX_PATH "/.miniGit/index"
#define HEAD_PATH "/.miniGit/HEAD"
#define OBJECTS_PATH "/.miniGit/objects/"
#define REFS_PATH "/.miniGit/refs"
#define REFS_HEAD_PATH "/.miniGit/refs/head"
#define REFS_HEAD_ORIGIN "/.miniGit/refs/origin"
#define REFS_HEAD_MASTER_PATH "/.miniGit/refs/head/master"
#define TEMP_COMMIT_TREE_PATH "/.miniGit/tempCommitTree"
#define TEMP_COMMIT_PATH "/.miniGit/tempCommit"
#define COLOR_BOLD_GREEN "\033[1;32m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_BOLD_BLUE "\033[1;34m"
#define COLOR_RESET "\033[0m"

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
#include <openssl/sha.h>
#include <fcntl.h>
#include <assert.h>

// Declaration of variables, structs, enums, ...
typedef struct {
    int sock_fd;
    struct sockaddr_in sock_addr;
    char username[USERLEN];
    char password[USERLEN];
    bool signedIn;
} clientInfo_t;

enum Command {      // Attention: if this is modified, function typed2enum should also be modified
    c_signin,
    c_signout,
    c_signup,
    c_response,
    c_file,
    c_block,
    c_push,
    c_pull,
    c_add,
    c_commit,
    c_init,
    c_wrongCommand,
    c_checkout,
    c_help,           // no packets with this command, but its useful for using a switch in client.c
    c_stop,           // no packets with this command, but its useful for using a switch in client.c
    c_clearScreen,    // no packets with this command, but its useful for using a switch in client.c
    c_ls              // no packets with this command, but its useful for using a switch in client.c
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

struct __attribute__((packed)) PullArgs{
    char username[USERLEN];
};

struct __attribute__((packed)) Packet{
    struct Header header;
    union Payload {
        struct SignArgs signArgs; // Used for signin or signup
        struct ResponseArgs responseArgs;
        struct FileArgs fileArgs;
        struct BlockArgs blockArgs;
        struct PullArgs pullArgs;
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
void setPullPacket(struct Packet *packet, const char *user);
void setFilePacket(struct Packet *packet, const char *fileName, uint32_t fileSize, bool lastFile);
void setBlockPacket(struct Packet *packet, uint32_t blockLength, const char *blockData);
enum Command getPacketCommand(struct Packet *packet);
enum ResponseValues getPacketResponseVal(struct Packet *packet);

void setClientUser(clientInfo_t *clientInfo, char *username);
void setClientPass(clientInfo_t *clientInfo, char *password);
void setClientSignedIn(clientInfo_t *clientInfo, bool signedIn);
bool isSignedIn(clientInfo_t *clientInfo);
int isRegistered(char *username);
int isPassCorrect(clientInfo_t *clientInfo);

int readNthLineFromFile(const char *srcPath, char *dest, int nthLine);
void getStdInput(char *dest, uint maxLength, clientInfo_t *clientInfo, const char * msg);
int createUser(clientInfo_t *clientInfo);
int createDir(const char *dirName);
enum Command typed2enum(const char *typedInCommand);
void printFile(const char *filePath);
int sendDir(int socket, struct Packet *packet, const char *dirName, int level, bool send ,bool print, int exclude);
int sendFile(int socket, struct Packet *packet, const char *filename, int exclude);
int recvDir(int socket, struct Packet *packet, const char *rootDir);
int recvFile(int socket, struct Packet *packet, uint32_t fileSize, char *filePath);
bool startsWith(const char *a, const char *b);
int countOccurrences(char c, const char *string);
int remove_directory(const char *path, const char *exclude);
int clean_directory(const char *path, const char *exclude, char* miniGitPath);
void getNthArg(const char * typedInCommand, int n, char * nthArg);
void getMsg(const char * typedInCommand, char * msg, int msgLen);
void printHelp(void);
void clearScreen(void);


// -----------------------------------------------------------------------------------------------
// Backend functions
void checkFileExistence(char *basePath,char* fileToFind, bool* findStatus);
int simpleCheckFileExistance(char *filePath);
void createFolder(char* prevFolderPath, char* folderName);
void createFile(char* folderPath, char* fileName, char* content);
void findObject(char *basePath,char* hashToFind, char* pathFound, int* findStatus);
void computeSHA1(char* text, char* hash_output);
void addObjectFile(char* hashName, char* contentPlusHeader, clientInfo_t *clientInfo);
void buildHeader(char type, long contentSize, char* header_out);
void hashObject(char type, char* path, char* fileName, char* hashName_out, char* creationFlag, clientInfo_t *clientInfo);
void hashObjectFromString(char type, char* content, char* hashName_out, char* creationFlag, clientInfo_t * clientInfo);
int getFieldsFromIndex(char* index_line, char* hash_out, char* path_out);
void getFieldsFromCommit(char* commitPath, char* tree, char* prevCommit, char* user);
int getFileLevel(char* path);
char* getContentFromObject(char* objectPath, char* type);
void getNameFromPath(char* path, char* name);
void buildCommitTree(char* commitTreeHash, char* creationFlag, clientInfo_t *clientInfo);
void getActiveBranch(char* branch_path, clientInfo_t *clientInfo);
void setActiveBranch(char* branchName,clientInfo_t *clientInfo);
void updateBranch(char* branchPath, char* newCommitHash);
void getLastCommit(char* commitFatherHash, clientInfo_t *clientInfo);
void getCommitFather(char* commitFatherHash);
bool checkUpToDate(clientInfo_t *clientInfo);
int buildCommitObject(char* commitMessage, char* commitTreeHash, char* user, char* commitHashOut, clientInfo_t *clientInfo);
void updateIndex(char* hash, char* path, char type, clientInfo_t *clientInfo);
void addAllFiles(char* basePath,char* prevFolder, int level, clientInfo_t *clientInfo);
int buildUpDir(char* currenDirPath, char* currentTreeHash, clientInfo_t *clientInfo);

// -----------------------------------------------------------------------------------------------
// User functions 
void init(clientInfo_t *clientInfo);
void add(clientInfo_t *clientInfo);
void commit(char* msg, clientInfo_t *clientInfo);
void cat_file(char* SHA1File, char* cat_type); /* "-p": content; "-t: type" */ // EASY TODO
void hash_object(char* fileName, char* optionalArgs); /* -w: add object */ // EASY TODO
int checkout(char* version, clientInfo_t *clientInfo, char* mssg);
//void log(char* SHA1Commit); /* By default, master */ //TODO

#endif