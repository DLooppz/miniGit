#include "miniGitUtils.h"

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
    packet->header.command = c_signup;
    packet->header.length = sizeof(struct Header) + sizeof(struct SignArgs);
    strcpy(packet->payload.signArgs.username, user);
    strcpy(packet->payload.signArgs.password, pass);
}

void setSignInPacket(struct Packet *packet, const char *user, const char *pass){
    bzero(packet,sizeof(packet));
    packet->header.command = c_signin;
    packet->header.length = sizeof(struct Header) + sizeof(struct SignArgs);
    strcpy(packet->payload.signArgs.username, user);
    strcpy(packet->payload.signArgs.password, pass);
}

void setResponsePacket(struct Packet *packet, enum ResponseValues responseValue){
    bzero(packet,sizeof(packet));
    packet->header.command = c_response;
    packet->header.length = sizeof(struct Header) + sizeof(struct ResponseArgs);
    packet->payload.responseArgs.value = responseValue;
}

void setSignOutPacket(struct Packet *packet){
    bzero(packet,sizeof(packet));
    packet->header.command = c_signout;
    packet->header.length = sizeof(struct Header);
}

void setPushPacket(struct Packet *packet){
    bzero(packet,sizeof(packet));
    packet->header.command = c_push;
    packet->header.length = sizeof(struct Header);
}

void setPullPacket(struct Packet *packet, const char *user){
    bzero(packet,sizeof(packet));
    packet->header.command = c_pull;
    packet->header.length = sizeof(struct Header) + sizeof(struct PullArgs);
    strcpy(packet->payload.pullArgs.username, user);
}

void setFilePacket(struct Packet *packet, const char *fileName, uint32_t fileSize, bool lastFile){
    bzero(packet,sizeof(packet));
    packet->header.command = c_file;
    packet->header.length = sizeof(struct Header) + sizeof(struct FileArgs);
    strcpy(packet->payload.fileArgs.name, fileName);
    packet->payload.fileArgs.fileSize = fileSize ;
    packet->payload.fileArgs.lastFile = lastFile ;
}

void setBlockPacket(struct Packet *packet, uint32_t blockLength, const char *blockData){
    bzero(packet,sizeof(packet));
    packet->header.command = c_block;
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

void getStdInput(char *dest, uint maxLength, clientInfo_t *clientInfo, const char * msg, bool hide){
    printf(COLOR_BOLD_GREEN);
    bzero(dest,sizeof(dest));
    if(isSignedIn(clientInfo))
        printf("%s@miniGit: ",clientInfo->username);
    else
        printf("@miniGit: ");
    if(msg!=NULL && strcmp(msg,"")!=0)
        printf("%s> ",msg);
    printf(COLOR_RESET);

    if(!hide){
        if (fgets(dest, maxLength, stdin) == NULL)
            dest[0] = '\0';
        else
            dest[strlen(dest)-1] = '\0';
    } else {
        static struct termios old_terminal;
        static struct termios new_terminal;

        //get settings of the actual terminal
        tcgetattr(STDIN_FILENO, &old_terminal);

        // do not echo the characters
        new_terminal = old_terminal;
        new_terminal.c_lflag &= ~(ECHO);

        // set this as the new terminal options
        tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);

        // get the password
        // the user can add chars and delete if he puts it wrong
        // the input process is done when he hits the enter
        // the \n is stored, we replace it with \0
        if (fgets(dest, maxLength, stdin) == NULL)
            dest[0] = '\0';
        else
            dest[strlen(dest)-1] = '\0';

        // go back to the old settings
        tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
        printf("\n");
    }
}

enum Command typed2enum(const char *typedInCommand){ // Attention: if this is modified, enum Command should also be modified
    if(strcmp(typedInCommand,"signin")==0 || strcmp(typedInCommand,"login")==0)
        return c_signin;
    else if(strcmp(typedInCommand,"signout")==0 || strcmp(typedInCommand,"logout")==0)
        return c_signout;
    else if(strcmp(typedInCommand,"signup")==0 || strcmp(typedInCommand,"register")==0)
        return c_signup;
    else if(strcmp(typedInCommand,"push")==0)
        return c_push;
    else if(strcmp(typedInCommand,"help")==0)
        return c_help;
    else if(strcmp(typedInCommand,"exit")==0 || strcmp(typedInCommand,"stop")==0)
        return c_stop;
    else if(strcmp(typedInCommand,"clear")==0)
        return c_clearScreen;
    else if(strcmp(typedInCommand,"ls")==0)
        return c_ls;
    else if(strcmp(typedInCommand,"init")==0)
        return c_init;
    else if(typedInCommand[0] == '\0')
        return c_enter;

    // If its none of the previous commands:
    char *token, *rest, aux[COMMANDLEN];
    strcpy(aux, typedInCommand);
    rest = aux;
    token = strtok_r(rest, " ", &rest);
    
    if(strcmp(token,"pull")==0 || strcmp(token,"clone")==0)
        return c_pull;
    else if(strcmp(token,"add")==0)
        return c_add;
    else if(strcmp(token,"commit")==0)
        return c_commit;
    else if(strcmp(token,"checkout")==0)
        return c_checkout;
    else if(strcmp(token,"cat-file")==0)
        return c_cat_file;
    else if(strcmp(token,"log")==0)
        return c_log;
    return c_wrongCommand;
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
// If print is true, the function prints the directory except files inside .miniGit. Also, it doesnt "go" into the folder .miniGit, so if print and send are true, files inside .miniGit will no be send. On the other hand, if print is false and send is true, everything will be send, including .miniGit folder
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
            if (print && startsWith(entry->d_name, ".miniGit"))
                continue;
            
            snprintf(path, sizeof(path), "%s/%s", dirName, entry->d_name);
            if(print){
                printf(COLOR_BOLD_BLUE);
                printf("%*s%s\n", (level * 2), "", entry->d_name);
                printf(COLOR_RESET);
            }
            ret = sendDir(socket, packet, path, level + 1, send, print, exclude); // recursion
            if(ret != 1)    
                return ret;
        } 
        // Else if entry is a file
        else {
            if(print) printf("%*s%s\n", (level * 2), "", entry->d_name);
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

        if(packet->header.command != c_file){
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
        if(packet->header.command != c_block){
            printf("Error: A block type packet was expected\n");
            return -1;
        }

        fprintf(fp, "%s", packet->payload.blockArgs.blockData);

        toReceive -= packet->payload.blockArgs.blockLength;
    }
    fclose(fp);
    return 1;
}

bool startsWith(const char *a, const char *b){
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
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

int clean_directory(const char *path, const char *exclude, char* miniGitPath) {
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
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..") || !strcmp(p->d_name, ".miniGit"))
                continue;

            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = clean_directory(buf, exclude, miniGitPath);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r && strcmp(path,exclude)!=0 && strcmp(path,miniGitPath) != 0)
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

void getMsg(const char * typedInCommand, char * msg, int msgLen){
    char *token, *rest, aux[COMMANDLEN];
    strcpy(aux, typedInCommand);
    rest = aux;

    int i = 0;
    strcpy(msg, "");
    while((token = strtok_r(rest, " ", &rest))){
        if( (msgLen - 1 - strlen(msg)) < strlen(token))
            break;

        if(i==1) 
            strcat(msg, token);
        else if(i!=0){
            strcat(msg, " ");
            strcat(msg, token);
        } 
        i++;
    }
}

void printHelp(void){
printf("------------------------------------------------------------------\n");
printf("------------------------------------------------------------------\n");
printf("                         Use of miniGit\n");
printf("------------------------------------------------------------------\n");
printf("------------------------------------------------------------------\n");
printf("Accepted commands:\n");
printf("%susername@miniGit: Enter command>%s help\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s init\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s clone USERNAME\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s clear\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s exit 'OR' stop\n",COLOR_GREEN, COLOR_RESET);
printf("\n");
printf("Accepted commands when not signed in (ONLY):\n");
printf("%s@miniGit: Enter command>%s login 'or' signin\n",COLOR_GREEN, COLOR_RESET);
printf("%s@miniGit: Enter command>%s signup 'or' register\n",COLOR_GREEN, COLOR_RESET);
printf("\n");
printf("Accepted commands when signed in (ONLY):\n");
printf("%susername@miniGit: Enter command>%s add or add .\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s commit YOUR MSG WITH WHITESPACES ALLOWED\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s log\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s checkout HASH 'or' checkout BRANCH\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s pull\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s push\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s ls\n",COLOR_GREEN, COLOR_RESET);
printf("%susername@miniGit: Enter command>%s logout 'or' signout\n",COLOR_GREEN, COLOR_RESET);
printf("------------------------------------------------------------------\n");
printf("------------------------------------------------------------------\n");
}

void clearScreen(void){
    printf("\e[1;1H\e[2J");
}

void getFileCreationTime(char *filePath, char *date)
{
    struct stat attr;
    stat(filePath, &attr);
    strcpy(date,ctime(&attr.st_mtime));
}

void checkFileExistence(char *basePath,char* fileToFind, bool* findStatus)
{
    /* Function that searchs fileToFind and stores "1" in findStatus if found */
    char path[1000];
    struct dirent *dirent_pointer;
    DIR *dir = opendir(basePath);
    
    // If current folder is being looked for
    if (strcmp(fileToFind, ".") == 0)
    {
        *findStatus = true;
        return;
    }    

    // Unable to open directory stream
    if (!dir)
        return;

    while (((dirent_pointer = readdir(dir)) != NULL) && (*findStatus != 1)){
        // Avoid "." and ".." entries
        if (strcmp(dirent_pointer->d_name, ".") != 0 && strcmp(dirent_pointer->d_name, "..") != 0){
            // Check if file was found
            if (strcmp(dirent_pointer->d_name,fileToFind) == 0){
                
                // Notify that file was found and return
                *findStatus = true;
                break;
            }

            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dirent_pointer->d_name);

            checkFileExistence(path,fileToFind,findStatus);
        }
    }
    closedir(dir);
    return;
}

int simpleCheckFileExistance(char *filePath){
    return access(filePath, F_OK);
}

void createFolder(char* prevFolderPath, char* folderName){

    /* 
    Creates folder inside prevFolderPath. If prevFolderPath doesnt exist, it may do unhappy things... (nothing happens)
    Nothing change if folderName already exist
    */

    char completePath[PATHS_MAX_SIZE];
    
    strcpy(completePath,prevFolderPath);
    strcat(completePath,"/");
    strcat(completePath,folderName);

    struct stat st = {0};
    if (stat(completePath, &st) == -1) {
        mkdir(completePath, 0700);
    }
}

void createFile(char* folderPath, char* fileName, char* content){

    /* 
    Creates fileName in folderPath and write content in it. 
    If  file already exist, it is overwritten
    If folderPath doesn't exist, it is not created and error is set
    */

    bool folderExist = false;
    bool fileExist = false;
    int fd;
    char completePath[PATHS_MAX_SIZE];

    // Make full path
    strcpy(completePath,folderPath);
    strcat(completePath, "/");
    strcat(completePath,fileName);

    // Create and write
    fd = open(completePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) perror("Error creating file. Probably containing folder doesn't exist\n"),exit(1);

    // Write content
    if (write(fd, content, strlen(content)) < 1) perror("Error writing content into file: "),exit(1);
    
    // Close
    if (close(fd) == -1) perror("Error closing file: "),exit(1);
}

void findObject(char *basePath,char* hashToFind, char* pathFound, int* findStatus)
{
    /* Function that finds path of hashToFind and stores it in pathFound */
    char path[1000];
    struct dirent *dirent_pointer;
    DIR *dir = opendir(basePath);
    char hashToFindCropped[2*SHA_DIGEST_LENGTH - 2 +1];
    char first2Characters[3];
    
    // printf("basePath%s\n",basePath);
    // printf("%s\n",hashToFind);

    // Be sure of hashName lenght (sometimes hashName comes with trash at the end)
    char hashToFind2[2*SHA_DIGEST_LENGTH+1];
    bzero(hashToFind2, sizeof(hashToFind2));
    bzero(hashToFindCropped, sizeof(hashToFindCropped));
    strncpy(hashToFind2,hashToFind,2*SHA_DIGEST_LENGTH);

    // Crop the first 2 characters of hashToFind
    for (int i=0;i<(2*SHA_DIGEST_LENGTH-2);i++){
        hashToFindCropped[i] = hashToFind2[i+2];
    }
    hashToFindCropped[strlen(hashToFindCropped)] = '\0';
    // printf("  %s\n\n",hashToFindCropped);

    // Unable to open directory stream
    if (!dir)
        return;

    while (((dirent_pointer = readdir(dir)) != NULL) && (*findStatus != 1)){
        // Avoid "." and ".." entries
        if (strcmp(dirent_pointer->d_name, ".") != 0 && strcmp(dirent_pointer->d_name, "..") != 0){
            // Check if file was found
            if (strcmp(dirent_pointer->d_name,hashToFindCropped) == 0){
                
                // Stores path if "pathFound" != NULL
                if (pathFound != NULL){
                    strcpy(hashToFindCropped,dirent_pointer->d_name);
                    strcpy(pathFound,basePath);
                    strcat(pathFound, "/");
                    strcat(pathFound, hashToFindCropped);
                }

                // Notify that file was found and return
                *findStatus = 1;
                break;
            }

            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dirent_pointer->d_name);

            findObject(path,hashToFind,pathFound,findStatus);
            if (*findStatus == 1)
                break;
        }
    }
    closedir(dir);
    return;
}

void computeSHA1(char* text, char* hash_output){
    /* Computes hash (SHA1 checksum) of text given by text*/
    
    unsigned char hashed_text[SHA_DIGEST_LENGTH];
    char hex_hashed_text[2*SHA_DIGEST_LENGTH] = {0,};

    // Hash the file 
    SHA1(text,strlen(text),hashed_text);

    // Convert to hex
    for(int i=0;i<SHA_DIGEST_LENGTH;i++){
        sprintf(hex_hashed_text + i * 2, "%02x", hashed_text[i]); 
    }

    // Save to output
    strcpy(hash_output,hex_hashed_text);
}

void addObjectFile(char* hashName, char* contentPlusHeader, clientInfo_t *clientInfo){
    /* Create fileObject in .miniGit/objects/ given its hash previously calculated and full content (header+content) */

    // Be sure of hashName lenght (sometimes hashName comes with trash at the end)
    char hashToAdd[2*SHA_DIGEST_LENGTH];
    strncpy(hashToAdd,hashName,2*SHA_DIGEST_LENGTH);
    
    // Dir name
    char objects_path[SMALLPATHLEN], dirName[SMALLPATHLEN];
    strcpy(objects_path, clientInfo->username);
    strcat(objects_path, OBJECTS_PATH);
    strcpy(dirName, objects_path);
    char aux[3];
    for (int i=0;i<2;i++){
        aux[i] = hashToAdd[i];
    }
    aux[2] = '\0';
    strcat(dirName,aux);

    // Create dir
    struct stat st = {0};
    if (stat(dirName, &st) == -1) {
        mkdir(dirName, 0700);
    }

    // File name
    char croppedHash[2*SHA_DIGEST_LENGTH - 2];
    char fullPath[2*SHA_DIGEST_LENGTH + 25];
    FILE *file_pointer;

    for (int i=0;i<(2*SHA_DIGEST_LENGTH - 2);i++){
        croppedHash[i] = hashToAdd[i+2];
    }

    strcpy(fullPath,dirName);
    strcat(fullPath,"/");
    strcat(fullPath,croppedHash);

    file_pointer  = fopen (fullPath, "wb");
    fprintf(file_pointer, "%s", contentPlusHeader);
    fclose(file_pointer);
}

void buildHeader(char type, long contentSize, char* header_out){

    char aux[256];
    sprintf(aux, "%ld", contentSize);
    
    switch (type){
    case 'c':
        strcpy(header_out,"commit ");
        break;
    case 't':
        strcpy(header_out,"tree ");
        break;
    case 'b':
        strcpy(header_out,"blob ");
        break;
    default:
        printf("Error in buildHeader: Incorrect Type.\n");
        exit(1);
    }

    // Cat size and "\0"
    strcat(header_out,aux);
    strcat(header_out,"\n");
}

void hashObject(char type, char* path, char* fileName, char* hashName_out, char* creationFlag, clientInfo_t *clientInfo){
    /* 
    char type: 'b':blob ; 't': tree ; 'c': commit
    char* path: path to file
    char* hashName_out: SHA1 computed hash of header + content of file
    char* creationFlag: "-w": create object ; Other: don't create
    */

    FILE *original_file;
    long contentSize;
    long headerSize;
    char *content, *contentPlusHeader;
    char header[75];

    // Open files (b: security for non Linux env)
    original_file = fopen (path , "rb");
    if(!original_file) 
    {
        // Most probably folder is empty. So tempTree was never opened. Dont create objects for empty folders
        strcpy(hashName_out,"NONE");
        printf("Dir entry '%s' was not added: Probably it was an empty folder.\n",fileName);
        return;
    }

    // Get size of file
    fseek(original_file , 0L , SEEK_END); /* Set the file position indicator at the end */
    contentSize = ftell(original_file); /* Get the file position indicator */
    rewind(original_file);

    // Build header 
    buildHeader(type,contentSize-1,header);
    headerSize = strlen(header);

    // allocate memory for content
    content = calloc(1, contentSize);
    if(!content) fclose(original_file),fputs("memory alloc fails",stderr),exit(1);

    // copy the file content into the content (string containing the whole text ; Omit \0)
    if(fread( content , contentSize, 1 , original_file) != 1)
        fclose(original_file),free(content),fputs("entire read fails",stderr),exit(1);

    // Concatenate content + header
    contentPlusHeader = calloc(1, contentSize + headerSize +1);
    if(!contentPlusHeader) fclose(original_file),free(content),fputs("memory alloc fails",stderr),exit(1);
    bzero(contentPlusHeader, sizeof(contentPlusHeader));
    strcat(contentPlusHeader,header);
    strcat(contentPlusHeader,content);

    // Compute SHA1 of header + content of file
    computeSHA1(contentPlusHeader,hashName_out);

    // To see header+content uncomment:
    // printf("\nHeader+Content: %s\n\n",contentPlusHeader);

    // Creation flag given
    if (strcmp(creationFlag,"-w") == 0){
        
        // Check if object already exist (findFlag will be 1)
        int findFlag = 0;
        char objects_path[SMALLPATHLEN];
        strcpy(objects_path, clientInfo->username);
        strcat(objects_path, OBJECTS_PATH);
        findObject(objects_path,hashName_out,NULL,&findFlag);

        // Create if doesnt exist
        if (findFlag != 1){
            addObjectFile(hashName_out, contentPlusHeader, clientInfo);
            switch (type){
            case 'b':
                printf("Blob object of '%s' added. (Hash: %s)\n",fileName,hashName_out);
                break;
            case 't':
                printf("Tree object of '%s' added. (Hash: %s)\n",fileName,hashName_out);
                break;
            case 'c':
                printf("Commit object added. (Hash: %s)\n",hashName_out);
                break;
            default:
                break;
            }
        }
    }

    // Close and free 
    fclose(original_file);
    free(content);
    free(contentPlusHeader);
}

void hashObjectFromString(char type, char* content, char* hashName_out, char* creationFlag, clientInfo_t * clientInfo){
    char header[75];
    char contentPlusHeader[strlen(content)+75];
    
    // Build header 
    buildHeader(type,strlen(content) - 1,header);

    // Concat content + header
    strcpy(contentPlusHeader,header);
    strcat(contentPlusHeader,content);

    // Compute SHA1 of header + content
    computeSHA1(contentPlusHeader,hashName_out);

    // Creation flag given
    if (strcmp(creationFlag,"-w") == 0){
        
        // Check if object already exist (findFlag will be 1)
        int findFlag = 0;
        char objects_path[SMALLPATHLEN];
        strcpy(objects_path, clientInfo->username);
        strcat(objects_path, OBJECTS_PATH);
        findObject(objects_path,hashName_out,NULL,&findFlag);
        
        // Create if doesnt exist
        if (findFlag != 1){
            addObjectFile(hashName_out, contentPlusHeader, clientInfo);
            switch (type){
            case 'b':
                printf("Blob object added. (Hash: %s)\n",hashName_out);
                break;
            case 't':
                printf("Tree object added. (Hash: %s)\n",hashName_out);
                break;
            case 'c':
                printf("Commit object added. (Hash: %s)\n",hashName_out);
                break;
            default:
                fprintf(stderr,"Error in hashObjectFromString. Bad type\n");
                exit(1);
            }
        }
    }

}

int getFieldsFromIndex(char* index_line, char* hash_out, char* path_out){
    /* 
    Get hash and path from index_line.
    No need to free memory from outside.
    Returns file type "F" or "D"
    */
            
    char *token, *string, *tofree;
    char *line[3];
    int i,ret;

    // Extracted from man 3 strsep
    tofree = string = strdup(index_line);
    assert(string != NULL);

    // Get type, hash and path (Hardcoded three tokens ALWAYS. Error if not)
    i = 0;
    while ((token = strsep(&string, " ")) != NULL){
        line[i] = token;
        i++;
    }

    // Store fields and return file type
    strcpy(hash_out,line[1]);
    strcpy(path_out,line[2]);

    if ( strcmp(line[0],"F") == 0 )
        ret = 0;
    else if ( strcmp(line[0],"D") == 0 )
        ret = 1;
    else
        printf("Error in index file. Corrupted!\n"), exit(EXIT_FAILURE);

    // free allocated mem
    free(tofree);
    return ret;
}

void getFieldsFromCommit(char* commitPath, char* tree, char* prevCommit, char* user, char* message){
    /*
    Function that returns fields of commit object.
    If NULL is passed as argument of some fields, they're not stored
    */

    FILE* commit;
    char buffer[PATHS_MAX_SIZE];
    char *token, *string, *tofree;
    int i;

    commit = fopen (commitPath , "rb");
    if(!commit) perror("Error opening commit: "),exit(1);

    // Get each line
    for (i=0;i<6;i++)
    {
        bzero(buffer, sizeof(buffer));
        fgets(buffer, PATHS_MAX_SIZE, commit);
        switch (i)
        {
            // Header
            case 0:
                break;

            // Tree    
            case 1:
                
                tofree = string = strdup(buffer);
                assert(string != NULL);
                
                token = strsep(&string, " ");
                if (tree != NULL)
                    strcpy(tree,string);
                free(tofree);
                break;

            // Commit    
            case 2:

                tofree = string = strdup(buffer);
                assert(string != NULL);

                token = strsep(&string, " ");
                if (prevCommit != NULL)
                    strcpy(prevCommit,string);
                free(tofree);
                break;

            // user    
            case 3:

                tofree = string = strdup(buffer);
                assert(string != NULL);

                token = strsep(&string, " ");
                if (user != NULL)
                    strcpy(user,string);
                free(tofree);
                break;

            // Emptyline    
            case 4:
                break;

            // Message    
            case 5:
                tofree = string = strdup(buffer);
                assert(string != NULL);


                token = strsep(&string, ": ");
                if (message != NULL)
                    strcpy(message,string);
                free(tofree);
                break;
        }       
    }

    fclose(commit);
}


int getFileLevel(char* path){
    /* Returns file level in working dir as an int (First level: 0) */
    char *token, *string, *tofree;
    int level = -1;

    tofree = string = strdup(path);
    assert(string != NULL);

    while ((token = strsep(&string, "/")) != NULL){
        level++;
    }
    
    free(tofree);
    free(string);
    return level-1;
}

char* getContentFromObject(char* objectPath, char* type){

    /* 
    Returns content of object (type == "-p") or header of object (type == "-t")
    If type is incorrect returns NULL
    ¡¡ Must free after use it !!
    */
    
    FILE* object;
    long contentSize;
    long headerSize = 75;
    char *content, *header;
    char buffer[1000];
    bzero(buffer,sizeof(buffer));
    int i;

    // Open object file
    object = fopen(objectPath,"rb");

    // Get its size (content + header)
    fseek(object , 0L , SEEK_END); /* Set the file position indicator at the end */
    contentSize = ftell(object); /* Get the file position indicator */
    rewind(object);

    // Alloc memory depending type 
    if (strcmp(type,"-p")==0)
        content = calloc(1,contentSize);
    else if (strcmp(type,"-t")==0)
        header = calloc(1,headerSize);
    else
    {
        printf("Bad type requested: '%s'. Expecting '%s' or '%s'\nNothing to cat.\n",type,"-p","-t");
        fclose(object);
        return NULL;
    }

    // Restore content depending type
    i = 0;
    while (fgets(buffer,1000,object))
    {
        if (i==0 && strcmp(type,"-t")==0)
        {
            strcat(header,buffer);
            header[strcspn(header, "\n")] = 0;
            fclose(object);
            return header;
        }
        else if (i>0 && strcmp(type,"-p")==0)
        {
            strcat(content,buffer);
        }
        i++;
    }
    
    // If code is here, must return content
    fclose(object);
    return content;
}


void getNameFromPath(char* path, char* name){
    /* Returns file name (last level in path) */
    char *token, *string, *tofree;
    char out[100];

    tofree = string = strdup(path);
    assert(string != NULL);

    while ((token = strsep(&string, "/")) != NULL){
        strcpy(out,token);
    }

    strcpy(name,out);
    free(tofree);
}

void buildCommitTree(char* commitTreeHash, char* creationFlag, clientInfo_t *clientInfo)
{
    /* The commit Tree is complete with the level zero files and dirs in index */
    FILE *index;
    char treeContent[TREE_MAX_SIZE];
    char buffer[PATHS_MAX_SIZE],type;
    char hash[2*SHA_DIGEST_LENGTH], path[PATHS_MAX_SIZE], filename[PATHS_MAX_SIZE]; /* filename has a '\n' at the end. It is cropped above */
    
    char index_path[SMALLPATHLEN];
    strcpy(index_path, clientInfo->username);
    strcat(index_path, INDEX_PATH);
    index = fopen(index_path,"rb");
    bzero(treeContent,sizeof(treeContent));

    while(fgets(buffer, PATHS_MAX_SIZE, index))
    {
        // FILE
        if ( getFieldsFromIndex(buffer,hash,path) == 0 ){
            // If file is in level 0, update tree
            if (getFileLevel(path) == 0)
            {
                getNameFromPath(path,filename);

                // Update entrie
                strcat(treeContent,"F ");
                strncat(treeContent,filename,strlen(filename)-1);
                strcat(treeContent," ");
                strcat(treeContent,hash);
                strcat(treeContent,"\n");
            }
            continue;
        }

        // DIR
        if ( getFieldsFromIndex(buffer,hash,path) == 1 ){
            // If dir is in level 0, update tree
            if (getFileLevel(path) == 0)
            {
                getNameFromPath(path,filename);

                // Update entries
                strcat(treeContent,"D ");
                strncat(treeContent,filename,strlen(filename)-1);
                strcat(treeContent," ");
                strcat(treeContent,hash);
                strcat(treeContent,"\n");
            }
        }

    }
    hashObjectFromString('t',treeContent,commitTreeHash,creationFlag, clientInfo);

    fclose(index);
}

void getActiveBranch(char* branch_path, clientInfo_t *clientInfo){
    FILE *head;

    // Open HEAD file and get branch path
    char head_path[SMALLPATHLEN];
    strcpy(head_path, clientInfo->username);
    strcat(head_path, HEAD_PATH);
    head = fopen(head_path,"rb");
    if (head == NULL) perror("Error opening branch file: "), exit(EXIT_FAILURE);

    fgets(branch_path,PATHS_MAX_SIZE,head);
    fclose(head);

    // Correct path (crop '\n' at the end)
    branch_path[strcspn(branch_path, "\n")] = 0;
}

void setActiveBranch(char* branchName,clientInfo_t *clientInfo){

    // Only needs to change HEAD file to points branchName
    FILE *head;
    char branchPath[PATHS_MAX_SIZE];

    // Open HEAD file and get branch path
    char head_path[SMALLPATHLEN];
    strcpy(head_path, clientInfo->username);
    strcat(head_path, HEAD_PATH);
    head = fopen(head_path,"wb");
    if (head == NULL) perror("Error opening branch file: "), exit(1);

    // Build branch path
    strcpy(branchPath,clientInfo->username);
    strcat(branchPath,REFS_HEAD_PATH);
    strcat(branchPath,"/");
    strcat(branchPath,branchName);

    // Update HEAD
    fputs(branchPath,head);
    fclose(head);
}

void updateBranch(char* branchPath, char* newCommitHash){
    
    int fd;

    fd = open(branchPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) perror("Error opening './someBranchToUpdate'"),exit(1);

    // Write new commit
    if (write(fd, newCommitHash, 2*SHA_DIGEST_LENGTH) < 1) perror("Error writing new commit hash into './someBranchToUpdate': "),exit(1);
    
    // Close
    if (close(fd) == -1) perror("Error closing './someBranchToUpdate': "),exit(1);
}

void getLastCommit(char* commitFatherHash, clientInfo_t *clientInfo){

    FILE *branch;
    char branch_path[PATHS_MAX_SIZE]; // It has '\n' at the end --> cropped

    getActiveBranch(branch_path, clientInfo);

    // Open active branch and get commit hash
    branch = fopen(branch_path,"rb");
    if (branch == NULL) perror("Error opening branch file: "), exit(EXIT_FAILURE);
    
    fgets(commitFatherHash,2*SHA_DIGEST_LENGTH+1,branch);
    // Correct hash (crop '\n' at the end)
    commitFatherHash[strcspn(commitFatherHash, "\n")] = 0;

    fclose(branch);
}

bool checkUpToDate(clientInfo_t *clientInfo){

    /* 
    Make a virtual tree of current staging area and compares with prev CommitTree 
    For first commit: return False
    For upToDate: return True
    For prevCommitTree != Current VirtualTree based in staging area: return False
    */
    
    char virtualTreeHash[2*SHA_DIGEST_LENGTH + 1];
    char prevCommitHash[2*SHA_DIGEST_LENGTH + 1];
    char prevCommit_path[PATHS_MAX_SIZE];
    char prevCommitTreeHash[2*SHA_DIGEST_LENGTH + 1];
    char prevCommitTree_path[PATHS_MAX_SIZE];
    int findStatus = 0;

    // Compute hash of virtual tree with staging area. Don't create it
    buildCommitTree(virtualTreeHash,"",clientInfo);

    // Get prev commit to look for its tree
    getLastCommit(prevCommitHash, clientInfo);
    
    // If not the first commit, search prev tree
    if (strcmp(prevCommitHash,"NONE") != 0)
    {
        char objects_path[SMALLPATHLEN];
        strcpy(objects_path, clientInfo->username);
        strcat(objects_path, OBJECTS_PATH);
        findObject(objects_path, prevCommitHash,prevCommit_path,&findStatus);
        getFieldsFromCommit(prevCommit_path,prevCommitTreeHash,NULL,NULL,NULL);
        
        // Remove posible '\n' at the end
        virtualTreeHash[strcspn(virtualTreeHash, "\n")] = 0;
        prevCommitTreeHash[strcspn(prevCommitTreeHash, "\n")] = 0;

        // If current virtualTreeHash and prev commitTreeHash are the same, return true: Up to date
        if (strcmp(prevCommitTreeHash,virtualTreeHash) == 0)
            return true;
        
        // Not upToDate: changes were made
        else
            return false;
    }

    // Not UpToDate: first commit
    return false;
}

int buildCommitObject(char* commitMessage, char* commitTreeHash, char* user, char* commitHashOut, clientInfo_t * clientInfo){

    // Check if repo is up to date
    if (checkUpToDate(clientInfo))
        return 0;
    
    int fd_commit;
    char prevCommitHash[2*SHA_DIGEST_LENGTH+1];
    char temp_commit_path[SMALLPATHLEN];
    strcpy(temp_commit_path,clientInfo->username);
    strcat(temp_commit_path,TEMP_COMMIT_PATH);

    // Get prev commit to append
    getLastCommit(prevCommitHash, clientInfo);

    // Create temporal file for commit object content
    fd_commit = open(temp_commit_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_commit < 0) perror("Error opening './tempCommitTree'"),exit(1);

    // Append commit tree
    if (write(fd_commit, "tree ", strlen("tree ")) < 1) perror("Error writing 'tree ' into './tempCommit': "),exit(1);
    if (write(fd_commit, commitTreeHash, strlen(commitTreeHash)) < 1) perror("Error writing 'commitTreeHash' into './tempCommit': "),exit(1);
    if (write(fd_commit, "\n ", 1) < 1) perror("Error writing '\\n' into './tempCommit': "),exit(1);

    // Append previous commit (First commit will have 'NONE')
    if (write(fd_commit, "commit ", strlen("commit ")) < 1) perror("Error writing 'commit ' into './tempCommit': "),exit(1);
    if (write(fd_commit, prevCommitHash, strlen(prevCommitHash)) < 1) perror("Error writing 'prevCommitHash' into './tempCommit': "),exit(1);
    if (write(fd_commit, "\n ", 1) < 1) perror("Error writing '\\n' into './tempCommit': "),exit(1);

    // Append commiter (user)
    if (write(fd_commit, "user ", strlen("user ")) < 1) perror("Error writing 'user' into './tempCommit': "),exit(1);
    if (write(fd_commit, user, strlen(user)) < 1) perror("Error writing 'user' into './tempCommit': "),exit(1);
    if (write(fd_commit, "\n ", 1) < 1) perror("Error writing '\\n' into './tempCommit': "),exit(1);

    // Append message
    if (write(fd_commit, "\nmessage: ", strlen("\nmessage: ")) < 1) perror("Error writing '\\nmessage: ' into './tempCommit': "),exit(1);
    if (write(fd_commit, commitMessage, strlen(commitMessage)+1) < 1) perror("Error writing 'user' into './tempCommit': "),exit(1);

    // Close 
    if (close(fd_commit) == -1) perror("Error closing './tempCommit': "),exit(1);

    // Add header, compute hash and create commit object
    hashObject('c',temp_commit_path,"",commitHashOut,"-w", clientInfo);

    // Remove temporal file
    if (remove(temp_commit_path) == -1) perror("Error trying to remove './tempCommit': "),exit(1);
    return 1;
}

void updateIndex(char* hash, char* path, char type, clientInfo_t *clientInfo){

    int fd;
    char index_path[SMALLPATHLEN];
    strcpy(index_path,clientInfo->username);
    strcat(index_path,INDEX_PATH);

    // Open index in append mode (Create if doesnt exist)
    fd = open(index_path, O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd < 0) perror("Error opening index: "), exit(1);
    
    // Write type 
    switch (type)
    {
    case 't':
        if (write(fd, "D ", strlen("D ")) < 1) perror("Error writing ' dir ' to index: "), exit(1);
        break;
    case 'b':
        if (write(fd, "F ", strlen("F ")) < 1) perror("Error writing ' file ' to index: "), exit(1);
        break;
    default:
        printf("Type error updating index\n"), exit(1);
        break;
    }

    // Write hash
    if (write(fd, hash, 2*SHA_DIGEST_LENGTH) < 1) perror("Error writing hash to index: "), exit(1);
    if (write(fd, " ", 1) < 1) perror("Error writing ' ' to index: "), exit(1);

    // Write path
    if (write(fd, path, strlen(path)) < 1) perror("Error writing path to index: "), exit(1);
    if (write(fd, "\n", strlen("\n")) < 1) perror("Error writing path to index: "), exit(1);

    
    if (close(fd) == -1) perror("Error closing index: "), exit(1);
}

void addAllFiles(char* basePath, char* prevFolder, int level, clientInfo_t *clientInfo)
{
    /* 
    Function that adds every file behind basePath. In first call:
    char* prevFolder: Any string, doesnt matter
    level: Must be 0;
    */ 

    char path[PATHLEN];
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    char tempTreeFileName[PATHLEN];
    int fd_tree;

    // Hash blob objects ------------------------------------------------
    if (!dir)
    {
        // Create blob object and update index
        char hashBlob[2*SHA224_DIGEST_LENGTH];
        hashObject('b',basePath,basePath,hashBlob,"-w", clientInfo);
        updateIndex(hashBlob,basePath,'b', clientInfo);

        // Add hash to tree father
        if (level > 1)
        {
            strcpy(tempTreeFileName,prevFolder);
            strcat(tempTreeFileName,"/tempTree");
            
            fd_tree = open(tempTreeFileName, O_RDWR | O_CREAT | O_APPEND, 0666);
            if (fd_tree < 0) perror("Error opening './tempTree': "), exit(1);
            
            if (write(fd_tree, hashBlob, strlen(hashBlob)) < 1) perror("Error writing hash into './tempTree': "), exit(1);
            if (write(fd_tree, "\n", 1) < 1) perror("Error writing ' ' into './tempTree': "), exit(1);
            
            if (close(fd_tree) == -1) perror("Error closing './tempTree' 1: "), exit(1);
        }
        return;
    }

    // "dir" is a directory: create trees for folders if level > 0
    if (level > 0){
        strcpy(tempTreeFileName,basePath);
        strcat(tempTreeFileName,"/tempTree");     
    }

    // Move over entries in basePath
    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0 && strcmp(dp->d_name, ".miniGit") != 0)
        {
            // These are entries in dir: update tempTreeFile if level > 0
            if (level > 0)
            {
                fd_tree = open(tempTreeFileName, O_RDWR | O_CREAT | O_APPEND, 0666);
                if (fd_tree < 0) perror("Error opening './tempTree' for entrie: "), exit(1);
                
                // Write type (dir or file)
                if (dp->d_type == DT_DIR)
                    if (write(fd_tree, "D ", strlen("D ")) < 1) perror("Error writing ' dir ' into './tempTree': "),exit(1);
                if (dp->d_type != DT_DIR)
                    if (write(fd_tree, "F ", strlen("F ")) < 1) perror("Error writing ' file ' into './tempTree': "),exit(1);

                // Write file name in temporal tree file
                if (write(fd_tree, dp->d_name, strlen(dp->d_name)) < 1) perror("Error writing entrie into './tempTree': "),exit(1);
                if (write(fd_tree, " ", 1) < 1) perror("Error writing entrie into './tempTree': "),exit(1);
                    
                // Clos temporal tree file
                if (close(fd_tree) == -1) perror("Error closing './tempTree' 2: "),exit(1);
            }
            
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);
            addAllFiles(path, basePath, level + 1, clientInfo);
        }
    }

    // No more entries: End of dir. Compute hash for tree and create TreeObject if level > 0
    if (level > 0)
    {
        // Compute folder hash and add to /objects
        char hashTree[2*SHA224_DIGEST_LENGTH];
        hashObject('t', tempTreeFileName, basePath, hashTree, "-w", clientInfo);
        
        // In case folder is not empty, update everything
        if (strcmp(hashTree,"NONE")!=0)
        {
            // Update index with folder and its hash
            updateIndex(hashTree,basePath,'t', clientInfo);
            
            // Remove temporal tree file
            if (remove(tempTreeFileName) == -1) perror("Error trying to remove './tempTree': "),exit(1);

            // Update treeFather with hash of this tree if level > 1
            if (level >1)
            {
                strcpy(tempTreeFileName,prevFolder);
                strcat(tempTreeFileName,"/tempTree");
                fd_tree = open(tempTreeFileName, O_RDWR | O_CREAT | O_APPEND, 0666);
                if (fd_tree < 0) perror("Error opening './tempTree': "), exit(1);

                if (write(fd_tree, hashTree, strlen(hashTree)) < 1) perror("Error writing hash into './tempTree': "),exit(1);
                if (write(fd_tree, "\n", 1) < 1) perror("Error writing ' ' into './tempTree': "),exit(1);

                if (close(fd_tree) == -1) perror("Error closing './tempTree' 1: "),exit(1);
            }
        }
        
    }
    closedir(dir);
}

int buildUpDir(char* currenDirPath, char* currentTreeHash, clientInfo_t *clientInfo){

    /* 
    Build up working directory from commitTree.
    currenDirPath in first call MUST BE ./USERNAME
    */
    char buffer[TREE_LINE_MAX_SIZE];
    char objectsPath[PATHS_MAX_SIZE];
    char treeObjectPath[PATHS_MAX_SIZE];
    char nextDir[PATHS_MAX_SIZE];
    char type[2],entryName[PATHS_MAX_SIZE];
    char pathToOldObject[PATHS_MAX_SIZE];
    char *contentOfFileToCreate;

    FILE* dirTree;
    int findStatus = 0,line = 0;

    char hash[2*SHA_DIGEST_LENGTH + 1];
    bzero(hash, sizeof(hash));
    bzero(objectsPath, sizeof(objectsPath));
    bzero(treeObjectPath, sizeof(treeObjectPath));
    
    // Set correct path for searching treeHash
    strcpy(objectsPath,clientInfo->username);
    strcat(objectsPath,"/.miniGit/objects");

    findObject(objectsPath,currentTreeHash,treeObjectPath,&findStatus);
    // printf("objectsPath: %s\n",objectsPath);
    // printf("treeObjectPath: %s\n",treeObjectPath);
    // printf("treeObjectHash: >%s<\n",currentTreeHash);
    // printf("\n");
    if (findStatus == 0)
    {
        // Some error (throw 0). Dir before checkout must be restored
        printf("Error0. Couldn't build up directory. Please, try again\n");
        return 0;
    }

    // Open tree object
    dirTree = fopen(treeObjectPath,"rb");

    while (fgets(buffer,TREE_LINE_MAX_SIZE,dirTree))
    {
        // Omit header
        if (line == 0)
        {
            line++;
            continue;
        }

        // Delete possible '\n' at the end of line
        buffer[strcspn(buffer, "\n")] = 0;

        // Get type, hash and name
        getNthArg(buffer,0,type);
        getNthArg(buffer,1,entryName);
        getNthArg(buffer,2,hash);

        // type == F: restore file
        if (strcmp(type,"F") == 0)
        {
            int findStatus2 = 0;
            findObject(objectsPath,hash,pathToOldObject,&findStatus2);
            if (findStatus2 == 0)
            {
                // Some error (throw 0). Dir before checkout must be restored
                printf("ErrorF. Couldn't build up directory. Please, try again\n");
                return 0;
            }
            // Get content --> dont forget to free!
            contentOfFileToCreate = getContentFromObject(pathToOldObject,"-p");
            createFile(currenDirPath,entryName,contentOfFileToCreate);
            free(contentOfFileToCreate);

            // Clean buffer
            bzero(buffer,sizeof(buffer));
            continue;
        }

        // type == D: restore dir downstream
        if (strcmp(type,"D") == 0)
        {
            int findStatus2 = 0;
            findObject(objectsPath,hash,pathToOldObject,&findStatus2);
            if (findStatus2 == 0)
            {
                // Some error (throw 0). Dir before checkout must be restored
                printf("ErrorD. Couldn't build up directory. Please, try again\n");
                return 0;
            }

            // Create the folder
            createFolder(currenDirPath,entryName);

            // Set paths for next level in dir
            strcpy(nextDir,currenDirPath);
            strcat(nextDir,"/");
            strcat(nextDir,entryName);
            if (buildUpDir(nextDir,hash,clientInfo) == 0)
            {
                // Some error downstream. Throw zero too
                return 0;
            }

            // Clean buffer
            bzero(buffer,sizeof(buffer));
        }

    }
    
    // Close tree object file
    fclose(dirTree);

    // Update Staging area
    // add(clientInfo);
    return 1;
}

// -----------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------
// User functions
// -----------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------

void init(clientInfo_t *clientInfo)
{
    // Define some paths
    char minigit_path[SMALLPATHLEN], refs_path[SMALLPATHLEN], refs_head_path[SMALLPATHLEN], refs_head_master_path[SMALLPATHLEN];
    strcpy(minigit_path, clientInfo->username);
    strcat(minigit_path, MINIGIT_PATH);
    strcpy(refs_path, clientInfo->username);
    strcat(refs_path, REFS_PATH);
    strcpy(refs_head_path, clientInfo->username);
    strcat(refs_head_path, REFS_HEAD_PATH);
    strcpy(refs_head_master_path, clientInfo->username);
    strcat(refs_head_master_path, REFS_HEAD_MASTER_PATH);

    // Check if .miniGit folder exist
    bool repo_exist = false;
    checkFileExistence(clientInfo->username,".miniGit",&repo_exist);
    if (repo_exist)
    {
        printf("A repository already exist in this folder. No init was made.\n");
        return;
    }

    // Create miniGit folders
    createFolder(".",clientInfo->username);
    createFolder(clientInfo->username,".miniGit/");
    createFolder(minigit_path,"objects/");
    createFolder(minigit_path,"refs/");
    createFolder(refs_path,"head/");
    createFolder(refs_path,"origin/");
    
    // Create miniGit files
    createFile(refs_head_path,"master","NONE");
    createFile(minigit_path,"index","NONE");
    createFile(minigit_path,"HEAD",refs_head_master_path);
    printf("Init finished\n");
}

void add(clientInfo_t *clientInfo){
    // Define some paths
    char index_path[SMALLPATHLEN];
    strcpy(index_path, clientInfo->username);
    strcat(index_path, "/.miniGit/index");
    // Try to delete old index file
    remove(index_path);

    // Add for all files
    addAllFiles(clientInfo->username, "", 0, clientInfo);
}

void commit(char* msg, clientInfo_t *clientInfo){
    char commitTreeHash[2*SHA_DIGEST_LENGTH], commitHash[2*SHA_DIGEST_LENGTH];
    char active_branch[PATHS_MAX_SIZE], branch_name[PATHS_MAX_SIZE];
    char index_path[SMALLPATHLEN];
    char buffer[PATHS_MAX_SIZE];
    int ret;
    FILE *index;
    
    // Check that index is not empty (NONE)
    strcpy(index_path, clientInfo->username);
    strcat(index_path, INDEX_PATH);
    index = fopen(index_path,"rb");
    fgets(buffer, PATHS_MAX_SIZE, index);
    buffer[strcspn(buffer, "\n")] = 0;
    if (strcmp(buffer,"NONE")==0)
    {
        printf("You must use the command `add` before `commit` in order to add files to the staging area\n");
        fclose(index);
        return;
    }

    // Check if repo is up to date
    if (checkUpToDate(clientInfo))
    {
        printf("Nothing to commit. Up to date\n");
        return;
    }

    // Create commit tree object
    buildCommitTree(commitTreeHash,"-w", clientInfo);

    // Create commit object
    buildCommitObject(msg, commitTreeHash, clientInfo->username, commitHash, clientInfo);

    // Update active branch
    getActiveBranch(active_branch, clientInfo);
    updateBranch(active_branch,commitHash);

    // Print succes messages
    getNameFromPath(active_branch,branch_name);
    printf("Active branch '%s' was updated with new commit!\n",branch_name);
}

int checkout(char* version, clientInfo_t *clientInfo, char* mssg)
{
    /* 
    Checkout to version. This argument can be:
        version: hash
        version: branchName
        If branch name lenght == 40 --> Complete mess! May Zeus help you
    */
    char commitHash[2*SHA_DIGEST_LENGTH+1];
    char treeHash[2*SHA_DIGEST_LENGTH+1];
    char commitPath[PATHS_MAX_SIZE];
    char currentBranchPath[PATHS_MAX_SIZE];
    char currentBranchName[PATHS_MAX_SIZE];
    char toActivateBranchPath[PATHS_MAX_SIZE]; /* Use only with branchs */
    char objects_path[PATHS_MAX_SIZE];
    char userDir[PATHS_MAX_SIZE];
    char miniGitPath[PATHS_MAX_SIZE];
    int restoreStatus;

    // Get current branch in case something goes wrong
    getActiveBranch(currentBranchPath,clientInfo);
    getNameFromPath(currentBranchPath,currentBranchName);

    // Case of hash to checkout
    if(strlen(version) == (2*SHA_DIGEST_LENGTH)){
        strcpy(commitHash,version);
        commitHash[strcspn(commitHash, "\n")] = 0;
    }

    // Case of branch
    else{
        char refsHeadPath[PATHS_MAX_SIZE];
        char branchPath[PATHS_MAX_SIZE];
        FILE* branchFile;

        // Build path for refs/head/
        strcpy(refsHeadPath,clientInfo->username);
        strcat(refsHeadPath,REFS_HEAD_PATH);
        version[strcspn(version, "\n")] = 0;

        // Check if branch exists
        strcpy(branchPath,refsHeadPath);
        strcat(branchPath,"/");
        strcat(branchPath,version);

        if (simpleCheckFileExistance(branchPath) == -1){
            sprintf(mssg,"The branch '%s' doesn't exist! Current branch '%s' remains active.\n", version, currentBranchName);
            fclose(branchFile);
            return 0;
        }
        
        // Branch exist: open branch path and get commit hash. Save branchPath
        strcpy(toActivateBranchPath,branchPath);
        branchFile = fopen(branchPath,"rb");
        fgets(commitHash,2*SHA_DIGEST_LENGTH+1,branchFile);
        commitHash[strcspn(commitHash, "\n")] = 0;

        // Check if commitHash is NONE
        if (strcmp(commitHash,"NONE") == 0){
            sprintf(mssg,"Branch '%s' has no initial commit. Current branch '%s' remains active\n", version, currentBranchName);
            fclose(branchFile);
            return 0;
        }

        fclose(branchFile);
    }

    // Up to now, version to checkout is in commitHash. Get its tree
    strcpy(objects_path,clientInfo->username);
    strcat(objects_path,OBJECTS_PATH);

    int findStatus = 0;
    findObject(objects_path, commitHash,commitPath,&findStatus);
    if (findStatus == 0)
    {
        // Some error finding commit object
        sprintf(mssg,"Version inserted doesnt exist. Current branch '%s' remains active\n", currentBranchName);
        return 0;
    }

    // Commit found. Get its tree hash
    getFieldsFromCommit(commitPath,treeHash,NULL,NULL,NULL);
    treeHash[strcspn(treeHash, "\n")] = 0;

    // // Now all dir must be deleted. Be sure that repo is up to date
    // if (checkUpToDate(clientInfo) == false)
    // {
    //     char userAnswer, buffer[10];
    //     printf("Warning!\nThere are changes that weren't commited. Last changes will be lost, are you sure you want to checkout? (y/n):  ");
    //     fgets(buffer, 10, stdin);
    //     userAnswer = buffer[0];
    //     switch (userAnswer)
    //     {
    //         case 'n':
    //             sprintf(mssg, "Checkout cancelled. Current branch '%s' remains active.\n", currentBranchName);
    //             return 0;
    //         case 'y':
    //             break;
    //         default:
    //             sprintf(mssg, "Checkout cancelled. Wrong answer ('y' or 'n' accepted). Current branch '%s' remains active.\n", currentBranchName);
    //             return 0;
    //     }
    // }

    // Clean complete dir
    strcpy(userDir,"./");
    strcat(userDir,clientInfo->username);
    strcpy(miniGitPath,userDir);
    strcat(miniGitPath,MINIGIT_PATH);

    clean_directory(userDir,userDir,miniGitPath);

    // Try to restore indicated version. If something fails, go back before checkout
    if (buildUpDir(userDir,treeHash,clientInfo) == 0)
    {
        // Some error building up dir. Restore version before checkout
        FILE* branchFile;
        int findStatus = 0;
        
        branchFile = fopen(currentBranchPath,"rb");
        fgets(commitHash,2*SHA_DIGEST_LENGTH,branchFile);
        commitHash[strcspn(commitHash, "\n")] = 0;
        fclose(branchFile); 

        findObject(objects_path, commitHash,commitPath,&findStatus);
        if (findStatus == 0)
        {
            sprintf(mssg,"A rare and insolit error ocurred. miniGit have abandoned you finding prevCommit. You must pull to recover your data... :0\n");
            return 0;
        }

        // Commit found. Get its tree hash
        getFieldsFromCommit(commitPath,treeHash,NULL,NULL,NULL);
        treeHash[strcspn(treeHash, "\n")] = 0;

        if (buildUpDir(userDir,treeHash,clientInfo) == 0){
            sprintf(mssg,"A rare and insolit error ocurred. miniGit have abandoned you restoring prevVersion. You must pull to recover your data... :0\n");
            return 0; 
        }

        // Prev version could be restored
        sprintf(mssg,"Checkout cancelled. Current branch '%s' remains active\ncommitHash: %s\n", currentBranchName,commitHash);
        return 0; 
    }

    // Here we're safe!

    // check old version: Dont update refs/head/CURRENT_BRANCH with prev version
    if (strlen(version) == 2*SHA_DIGEST_LENGTH)
    {
        sprintf(mssg,"Version %s was restored correctly!\n",version);
    }
    
    // CHANGING BRANCH: Update HEAD if branch name was given
    else
    {
        updateBranch(toActivateBranchPath,commitHash);
        setActiveBranch(version,clientInfo);
        sprintf(mssg,"Branch '%s' is active now!\n",version);
    }
    return 1;
}

int cat_file(char* objectHash, char* cat_type, clientInfo_t *clientInfo, char* mssg)
{
    /* Cat SHA1 object ; "-p": content; "-t: type" */
    
    char basePath[PATHS_MAX_SIZE];
    char objectPath[PATHS_MAX_SIZE];
    char *content; /* Remember to free */
    int findStatus = 0;

    // Set objects path
    bzero(basePath,sizeof(basePath));
    strcpy(basePath,clientInfo->username);
    strcat(basePath,"/.miniGit/objects");

    // Find object path
    findObject(basePath,objectHash,objectPath,&findStatus);
    if (findStatus == 0)
    {
        sprintf(mssg,"Object with hash %s doesn't exist!\n",objectHash);
        return 0;
    }

    // Get content
    content = getContentFromObject(objectPath,cat_type);
    if (content == NULL)
    {
        sprintf(mssg,"Please, try again\n");
        return 0;
    }

    // Print content requested:
    printf("\n%s\n",content);
    free(content);
    return 1;
}

void logHist(clientInfo_t *clientInfo)
{
    /* Print history from current active branch */
    char commitHash[2*SHA_DIGEST_LENGTH+1];
    char prevCommitHash[2*SHA_DIGEST_LENGTH+1];
    char user[MSGLEN], tree[MSGLEN], commitPath[PATHS_MAX_SIZE], mssg[MSGLEN], date[100];
    char objectsPath[PATHS_MAX_SIZE];
    int findStatus;

    // Build objects path
    strcpy(objectsPath,clientInfo->username);
    strcat(objectsPath,"/.miniGit/objects");
    
    // Get current commit (the one in head/refs/master)
    getLastCommit(commitHash,clientInfo);

    while (strcmp(commitHash,"NONE") != 0)
    {
        // Get commit path
        findStatus = 0;
        findObject(objectsPath,commitHash,commitPath,&findStatus);
        if (findStatus == 0)
        {
            printf("Unknown error. Try again\n");
            return;
        }

        // Get fields from commit and remove \n
        getFieldsFromCommit(commitPath,tree,prevCommitHash,user,mssg);
        tree[strcspn(tree, "\n")] = 0;
        prevCommitHash[strcspn(prevCommitHash, "\n")] = 0;
        user[strcspn(user, "\n")] = 0;
        mssg[strcspn(mssg, "\n")] = 0;

        // Get date of last modification 
        getFileCreationTime(commitPath,date);

        // Print logs
        printf("%sCommit: %s%s\n",COLOR_BOLD_BLUE,commitHash,COLOR_RESET);
        printf("Author: %s\n",user);
        printf("Date: %s\n",date);
        printf("   %s\n\n",mssg);

        // Check if prev commit is NONE
        if (strcmp(prevCommitHash,"NONE") == 0)
            break;
        
        // Update hash and reset 
        strcpy(commitHash,prevCommitHash);
        bzero(commitPath,sizeof(commitPath));
        bzero(tree,sizeof(tree));
        bzero(prevCommitHash,sizeof(prevCommitHash));
        bzero(user,sizeof(user));
        bzero(mssg,sizeof(mssg));
        bzero(date,sizeof(date));
    }
}