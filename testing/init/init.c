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
#include <assert.h>
#include <stdbool.h>

#define PATHS_MAX_SIZE 512
#define MINIGIT_PATH "./.miniGit/"
#define INDEX_PATH "./.miniGit/index"
#define HEAD_PATH "./.miniGit/HEAD"
#define OBJECTS_PATH "./.miniGit/objects/"
#define REFS_PATH "./.miniGit/refs"
#define REFS_HEAD_PATH "./.miniGit/refs/head"
#define REFS_HEAD_MASTER_PATH "./.miniGit/refs/head/master"

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

void init()
{
    // Check if .miniGit folder exist
    bool repo_exist = false;
    checkFileExistence(".",".miniGit",&repo_exist);
    if (repo_exist)
    {
        printf("A repository already exist in this folder. No init was made.\n");
        return;
    }

    // Create miniGit folders
    createFolder(".",".miniGit/");
    createFolder(MINIGIT_PATH,"objects/");
    createFolder(MINIGIT_PATH,"refs/");
    createFolder(REFS_PATH,"head/");
    createFolder(REFS_PATH,"origin/");
    
    // Create miniGit files
    createFile(REFS_HEAD_PATH,"master","NONE");
    createFile(MINIGIT_PATH,"index"," ");
    createFile(MINIGIT_PATH,"HEAD",REFS_HEAD_MASTER_PATH);

}

int main(void){

    init();
    return 0;
}