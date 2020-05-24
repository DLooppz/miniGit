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

// Example of hash: 1ef89d27a674ddaf252b2ca35bb74aff06eac15d


void findFile(char *basePath,char* fileToFind, char* foundFile)
{
    /* Function that finds fileToFind and stores it in foundFile. Returns 1 when found */
    char path[1000];
    struct dirent *dirent_pointer;
    DIR *dir = opendir(basePath);

    // Unable to open directory stream
    if (!dir)
        return;

    while (((dirent_pointer = readdir(dir)) != NULL) && (strcmp(foundFile,fileToFind) != 0)){
        // Avoid "." and ".." entries
        if (strcmp(dirent_pointer->d_name, ".") != 0 && strcmp(dirent_pointer->d_name, "..") != 0){
            // Check if file was found
            if (strcmp(dirent_pointer->d_name,fileToFind) == 0){
                strcpy(fileToFind,dirent_pointer->d_name);
                strcpy(foundFile,basePath);
                strcat(foundFile, "/");
                strcat(foundFile, fileToFind);
                strcpy(fileToFind,foundFile);
                return;
            }

            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dirent_pointer->d_name);

            findFile(path,fileToFind,foundFile);
        }
    }
    closedir(dir);
    return;
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
    char foundFile[150];
    findFile(".",hashObj,foundFile);

    // Open file and print its content
    printf("File was found!! Name: %s\n",foundFile);
    foundFileContent = fopen(foundFile,"rb");
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
