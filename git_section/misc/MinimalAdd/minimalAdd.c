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

void findFile(char *basePath,char* hashToFind, char* pathFound, int* findStatus)
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

            findFile(path,hashToFind,pathFound,findStatus);
        }
    }
    closedir(dir);
}

void computeSHA1(char* text, char* hash_output){
    /* Computes hash (SHA1 checksum) of text given by text*/
    
    unsigned char hashed_text[SHA_DIGEST_LENGTH];
    char hex_hashed_text[2*SHA_DIGEST_LENGTH] = {0,};

    // Hash the file 
    SHA1(text,sizeof(text),hashed_text);

    // Convert to hex
    for(int i=0;i<SHA_DIGEST_LENGTH;i++){
        sprintf(hex_hashed_text + i * 2, "%02x", hashed_text[i]); 
    }

    // Save to output
    strcpy(hash_output,hex_hashed_text);
}

void addObjectFile(char* hashName, char* contentPlusHeader){
    /* Create fileObject in .miniGit/objects/ given its hash previously calculated and full content (header+content) */

    // Be sure of hashName lenght (sometimes hashName comes with trash at the end)
    char hashToAdd[2*SHA_DIGEST_LENGTH];
    strncpy(hashToAdd,hashName,2*SHA_DIGEST_LENGTH);
    
    // Dir name
    char dirName[25] = ".miniGit/objects/";
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

    file_pointer  = fopen (fullPath, "w");
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

void hashObject(char type, char* path, char* fileName, char* hashName_out, char* creationFlag){
    /* 
    char type: 'b':blob ; 't': tree
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
        findFile(".miniGit/objects/",hashName_out,NULL,&findFlag);
        
        // Create if doesnt exist
        if (findFlag != 1){
            addObjectFile(hashName_out, contentPlusHeader);
            switch (type){
            case 'b':
                printf("Blob object of '%s' added. (Hash: %s)\n",fileName,hashName_out);
                break;
            case 't':
                printf("Tree object of '%s' added. (Hash: %s)\n",fileName,hashName_out);
                break;
            default:
                break;
            }
        }
        // else
        //     printf("Blob object of '%s' already exists. Not created. (Hash: %s)\n",path,hashName_out);
    }

    // Close and free 
    fclose(original_file);
    free(content);
    free(contentPlusHeader);
}

void updateIndex(char* hash, char* path, char type){

    int fd;

    // Open index in append mode (Create if doesnt exist)
    fd = open("./.miniGit/index", O_RDWR | O_CREAT | O_APPEND, 0666);
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

void addAllFiles(char* basePath,char* prevFolder, int level)
{
    /* 
    Function that adds every file behind basePath. In first call:
    char* prevFolder: Any string, doesnt matter
    level: Must be 0;
    */ 

    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    char tempTreeFileName[1000];
    int fd_tree;

    // Hash blob objects ------------------------------------------------
    if (!dir)
    {
        // Create blob object and update index
        char hashBlob[2*SHA224_DIGEST_LENGTH];
        hashObject('b',basePath,basePath,hashBlob,"-w");
        updateIndex(hashBlob,basePath,'b');

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
                
                // Write file name in temporal tree file
                if (write(fd_tree, dp->d_name, strlen(dp->d_name)) < 1) perror("Error writing entrie into './tempTree': "),exit(1);
                
                // Write type (dir or file)
                if (dp->d_type == DT_DIR)
                    if (write(fd_tree, " dir ", strlen(" dir ")) < 1) perror("Error writing ' dir ' into './tempTree': "),exit(1);
                if (dp->d_type != DT_DIR)
                    if (write(fd_tree, " file ", strlen(" file ")) < 1) perror("Error writing ' file ' into './tempTree': "),exit(1);
                    
                // Clos temporal tree file
                if (close(fd_tree) == -1) perror("Error closing './tempTree' 2: "),exit(1);
            }
            
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);
            addAllFiles(path,basePath, level + 1);
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
        
        if (write(fd_tree, "Level ", strlen("\nLevel ")) < 1) perror("Error writing '\\n' into './tempTree': "),exit(1);
        if (write(fd_tree, strLevel, strlen(strLevel)) < 1) perror("Error writing level into './tempTree': "),exit(1);
        
        if (close(fd_tree) == -1) perror("Error closing './tempTree' 3: "),exit(1);

        // Compute folder hash and add to /objects
        char hashTree[2*SHA224_DIGEST_LENGTH];
        hashObject('t',tempTreeFileName,basePath,hashTree,"-w");
        
        // Update index with folder and its hash
        updateIndex(hashTree,basePath,'t');
        
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

void add(int argc, char *argv[])
{

    // Bad usage
    if (argc < 2){
        printf("Error!\nUsage: './minimalAdd [File1]...[FileN]' -- Not folders allowed\n");
        printf("Usage: './minimalAdd .' -- All files and folders will be add\n");
        exit(1);
    }

    int n_files = argc - 1;

    // Bad usage
    if ((n_files > 1) && (strcmp(argv[1],".")==0)){
        printf("Error!\nUsage: './minimalAdd [File1]...[FileN]' -- Not folders allowed\n");
        printf("Usage: './minimalAdd .' -- All files and folders will be add\n");
        exit(1);
    }

    // Try to delete old index file
    if (remove("./.miniGit/index") == -1) perror("Error trying to remove old './.miniGIT/index': ");

    // Add for some files
    if (n_files >= 1 && (strcmp(argv[1],".")!=0)){
        char hashedFile[2*SHA_DIGEST_LENGTH];
        for (int i=0;i<n_files;i++){
            hashObject('b',argv[i+1],argv[i+1],hashedFile,"-w");
        }
    }

    // Add for all files
    if ((n_files == 1) && (strcmp(argv[1],".")==0)){
        addAllFiles(".","",0);
    }


}


int main(int argc, char *argv[]){

    add(argc,argv);
    return 0;
}
