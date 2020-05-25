#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <openssl/sha.h>

/*
Util functions:
    struct dirent *readdir(DIR *dirp);
    DIR *opendir(const char *dirname);
    int closedir(DIR *dirp);
*/

/*
Recordar:
    argc: nro de argumentos (nombre del ej + args)
    arv[]: array de strings con los args (nombre del ej + extras)
*/

// Example of hash: 013bbe798a36e78ebf2a3cb96c597bd5919ed937


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
                strcpy(hashToFindCropped,dirent_pointer->d_name);
                printf("hashToFindCropped: %s\n",hashToFindCropped);
                strcpy(pathFound,basePath);
                printf("pathFound: %s\n",pathFound);
                strcat(pathFound, "/");
                printf("pathFound: %s\n",pathFound);
                strcat(pathFound, hashToFindCropped);
                printf("pathFound: %s\n",pathFound);

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



int main(int argc, char *argv[]){

    // Correct usage
    if (argc != 2){
        printf("Error!\nUsage: ./searchFile [HashToFind]\n\n");
        exit(1);
    }

    // Check correct hash input
    if (strlen(argv[1]) != 2*SHA_DIGEST_LENGTH){
        printf("Error!\nIncorrect SHA1 input. Must be in HEX (40 characters)\n\n");
        exit(2);
    }

    // File found
    FILE *foundFileContent;
    char *buffer;
    long lSize;

    // Get hash
    char hashObj[150];
    strcpy(hashObj,argv[1]);
    printf("Hash inserted: %s\n\n",hashObj);

    // Search in dirs for FileHashed
    char pathFound[150];
    int findStatus = 0;
    findFile(".",hashObj,pathFound,&findStatus);
    if (findStatus != 1){
        printf("Error! Hashed object not found!\n");
        printf("Found instead %s\n",pathFound);
        exit(EXIT_FAILURE);
    }

    // Open file and print its content
    printf("File was found!! Name: %s\n",pathFound);
    foundFileContent = fopen(pathFound,"rb");
    if(!foundFileContent) perror("Error opening file hashed\n"),exit(3);

    // Set the file position indicator at the end
    fseek(foundFileContent , 0L , SEEK_END);

    // Get the file position indicator 
    lSize = ftell(foundFileContent);
    rewind(foundFileContent);

    // allocate memory for entire content
    buffer = calloc(1, lSize + 1);
    if(!buffer) fclose(foundFileContent),fputs("memory alloc fails",stderr),exit(4);

    // copy the file into the buffer 
    if(fread( buffer , lSize, 1 , foundFileContent) != 1)
        fclose(foundFileContent),free(buffer),fputs("entire read fails",stderr),exit(5);

    // Now buffer is a string containing the whole text
    printf("********* Found file content:\n%s\n",buffer);

    free(buffer);
    fclose(foundFileContent);
    return 0;
}
