#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>




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
    case 'b':
        strcpy(header_out,"blob ");
        break;
    default:
        printf("Error in buildHeader: Incorrect Type.\n");
        exit(1);
    }

    // Cat size and "\0"
    strcat(header_out,aux);
    strcat(header_out,"\\0");
}

void hashObjectBlob(char* path, char* hashName_out, char* creationFlag){
    /* 
    char type: 'b':blob ; 't':tree
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
    buildHeader('b',lSize-1,header);
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
            printf("Blob object of '%s' added. (Hash: %s)\n",path,hashName_out);
        }
        else
            printf("Blob object of '%s' already exists. Not created. (Hash: %s)\n",path,hashName_out);
    }

    // Close and free 
    fclose(original_file);
    free(content);
    free(contentPlusHeader);
}

void hashObjectTree(char* path, char* hashName_out, char* creationFlag){




}

void addAllFiles(char *basePath, const int level)
{
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    // If not a directory add BlobObject and return. First basePath MUST be "."
    if (!dir){
        char hashBlob[2*SHA224_DIGEST_LENGTH];
        hashObjectBlob(basePath,hashBlob,"-w");
        return;
    }
    
    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0 && strcmp(dp->d_name, ".miniGit") != 0)
        {
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);
            addAllFiles(path, level + 2);
        }
    }

    closedir(dir);
}


void updateIndex(){



}

int main(int argc, char *argv[]){

    // Correct usage
    if (argc < 2){
        printf("Error!\nUsage: './minimalAdd [File1]...[FileN]' -- Not folders allowed\n");
        printf("Usage: './minimalAdd .' -- All files and folders will be add\n");
        exit(1);
    }

    // "." to add all files TODO
    int n_files = argc - 1;
    if ((n_files == 1) && (strcmp(argv[1],".")==0)){
        n_files = -1;
        printf("All files to add!\n\n");
    }
    // [files] in other case
    else{
        printf("Files to add: ");
        for (int i=0;i<n_files;i++){
            printf("%d)'%s' ",i+1,argv[i+1]);
        }
        printf("\n\n");
    }

    // Add for some files
    if (n_files > 0){
        char hashedFile[2*SHA_DIGEST_LENGTH];
        for (int i=0;i<n_files;i++){
            hashObjectBlob(argv[i+1],hashedFile,"-w");
        }
    }

    // Add for all files
    if (n_files<0){
        addAllFiles(".",0);
    }

    

    return 0;
}
