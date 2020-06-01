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
printf("username@miniGit: Enter command> help\n");
printf("username@miniGit: Enter command> init\n");
printf("username@miniGit: Enter command> add or add .\n");
printf("username@miniGit: Enter command> commit YOUR MSG WITH WHITESPACES ALLOWED\n");
printf("username@miniGit: Enter command> checkout HASH 'or' checkout BRANCH\n");
printf("username@miniGit: Enter command> clone USERNAME\n");
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
                return;
            }

            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dirent_pointer->d_name);

            checkFileExistence(path,fileToFind,findStatus);
        }
    }
    closedir(dir);
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
    char hashToFindCropped[2*SHA_DIGEST_LENGTH - 2];
    char first2Characters[3];
    
    // Crop the first 2 characters of hashToFind
    for (int i=0;i<(2*SHA_DIGEST_LENGTH-2);i++){
        hashToFindCropped[i] = hashToFind[i+2];
    }

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
                return;
            }

            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dirent_pointer->d_name);

            findObject(path,hashToFind,pathFound,findStatus);
        }
    }
    closedir(dir);
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
    long lSize;
    long headerSize;
    char *content, *contentPlusHeader;
    char header[75];

    // Open files (b: security for non Linux env)
    original_file = fopen (path , "rb");
    if(!original_file) perror("Error opening file to create object"),exit(1);

    // Get size of file
    fseek(original_file , 0L , SEEK_END); /* Set the file position indicator at the end */
    lSize = ftell(original_file); /* Get the file position indicator */
    rewind(original_file);

    // Build header 
    buildHeader(type,lSize-1,header);
    headerSize = strlen(header);

    // allocate memory for content
    content = calloc(1, lSize);
    if(!content) fclose(original_file),fputs("memory alloc fails",stderr),exit(1);

    // copy the file content into the content (string containing the whole text ; Omit \0)
    if(fread( content , lSize-1, 1 , original_file) != 1)
        fclose(original_file),free(content),fputs("entire read fails",stderr),exit(1);

    // Concatenate content + header
    contentPlusHeader = calloc(1, lSize + headerSize);
    if(!contentPlusHeader) fclose(original_file),free(content),fputs("memory alloc fails",stderr),exit(1);
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

void getFieldsFromCommit(char* commitPath, char* tree, char* prevCommit, char* user){
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
    for (i=0;i<4;i++)
    {
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

            // Commit    
            case 3:

                tofree = string = strdup(buffer);
                assert(string != NULL);


                token = strsep(&string, " ");
                if (user != NULL)
                    strcpy(user,string);
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

void buildCommitTree(char* commitTreeHash, clientInfo_t *clientInfo)
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
    printf("%s\n", treeContent);
    hashObjectFromString('t',treeContent,commitTreeHash,"-w", clientInfo);

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

void setActiveBranch(char* branchName){
    // TODO
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

int buildCommitObject(char* commitMessage, char* commitTreeHash, char* user, char* commitHashOut, clientInfo_t * clientInfo){

    int fd_commit,findStatus=0;
    char prevCommit[2*SHA_DIGEST_LENGTH];
    char prevCommit_path[PATHS_MAX_SIZE];
    char prevCommitTreeHash[2*SHA_DIGEST_LENGTH];
    char prevCommitTree_path[PATHS_MAX_SIZE];

    // Check if nothing has changed (up to date)
    getLastCommit(prevCommit, clientInfo);
    
    // If not the first commit, search prev tree
    if (strcmp(prevCommit,"NONE") != 0)
    {
        char objects_path[SMALLPATHLEN];
        strcpy(objects_path, clientInfo->username);
        strcat(objects_path, OBJECTS_PATH);
        findObject(objects_path, prevCommit,prevCommit_path,&findStatus);
        getFieldsFromCommit(prevCommit_path,prevCommitTreeHash,NULL,NULL);
        
        prevCommit_path[strcspn(commitTreeHash, "\n")] = 0;
        prevCommitTreeHash[strcspn(prevCommitTreeHash, "\n")] = 0;

        // If current CommitTreeHash and prev commitTreeHash are the same, return. Up to date
        if (strcmp(prevCommitTreeHash,commitTreeHash) == 0)
            return 0;
    }
    
    char temp_commit_path[SMALLPATHLEN];
    strcpy(temp_commit_path,clientInfo->username);
    strcat(temp_commit_path,TEMP_COMMIT_PATH);

    // Create temporal file for commit object content
    fd_commit = open(temp_commit_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_commit < 0) perror("Error opening './tempCommitTree'"),exit(1);

    // Append commit tree
    if (write(fd_commit, "tree ", strlen("tree ")) < 1) perror("Error writing 'tree ' into './tempCommit': "),exit(1);
    if (write(fd_commit, commitTreeHash, strlen(commitTreeHash)) < 1) perror("Error writing 'commitTreeHash' into './tempCommit': "),exit(1);
    if (write(fd_commit, "\n ", 1) < 1) perror("Error writing '\\n' into './tempCommit': "),exit(1);

    // Append previous commit (First commit will have 'NONE')
    if (write(fd_commit, "commit ", strlen("commit ")) < 1) perror("Error writing 'commit ' into './tempCommit': "),exit(1);
    if (write(fd_commit, prevCommit, strlen(prevCommit)) < 1) perror("Error writing 'prevCommit' into './tempCommit': "),exit(1);
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
        // Convert int level to string
        char strLevel[3];
        sprintf(strLevel, "%d", level-1);

        // Mark trees with their level at the end
        fd_tree = open(tempTreeFileName, O_RDWR | O_CREAT | O_APPEND, 0666);
        if (fd_tree < 0) perror("Error opening './tempTree' for level: "),exit(1);
        
        if (write(fd_tree, "\n", strlen("\n")) < 1) perror("Error writing '\\n' into './tempTree': "),exit(1);
        if (write(fd_tree, strLevel, 3) < 1) perror("Error writing level into './tempTree': "),exit(1);
        
        if (close(fd_tree) == -1) perror("Error closing './tempTree' 3: "),exit(1);

        // Compute folder hash and add to /objects
        char hashTree[2*SHA224_DIGEST_LENGTH];
        hashObject('t', tempTreeFileName, basePath, hashTree, "-w", clientInfo);
        
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
    closedir(dir);
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
    createFile(minigit_path,"index"," ");
    createFile(minigit_path,"HEAD",refs_head_master_path);

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
    int ret;
    
    // Create commit tree object
    buildCommitTree(commitTreeHash, clientInfo);

    // Create commit object
    ret = buildCommitObject(msg, commitTreeHash, clientInfo->username, commitHash, clientInfo);
    if (ret == 0){
        printf("Nothing to commit. Up to date\n");
        return;
    }

    // Update active branch
    getActiveBranch(active_branch, clientInfo);
    updateBranch(active_branch,commitHash);

    // Print succes messages
    getNameFromPath(active_branch,branch_name);
    printf("Active branch '%s' was updated with new commit!\n",branch_name);
}
