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
#include "miniGitUtils.h"



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
    SHA1(text,strlen(text),hashed_text);

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
    char dirName[25] = OBJECTS_PATH;
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

void hashObject(char type, char* path, char* fileName, char* hashName_out, char* creationFlag){
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
        findFile(OBJECTS_PATH,hashName_out,NULL,&findFlag);
        
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

void hashObjectFromString(char type, char* content, char* hashName_out, char* creationFlag){

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
        findFile(OBJECTS_PATH,hashName_out,NULL,&findFlag);
        
        // Create if doesnt exist
        if (findFlag != 1){
            addObjectFile(hashName_out, contentPlusHeader);
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

void buildCommitTree(char* commitTreeHash)
{
    /* The commit Tree is complete with the level zero files and dirs in index */

    FILE *index;
    char treeContent[TREE_MAX_SIZE];
    char buffer[PATHS_MAX_SIZE],type;
    char hash[2*SHA_DIGEST_LENGTH], path[PATHS_MAX_SIZE], filename[PATHS_MAX_SIZE]; /* filename has a '\n' at the end. It is cropped above */
    
    index = fopen(INDEX_PATH,"rb");

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

    hashObjectFromString('t',treeContent,commitTreeHash,"-w");

    fclose(index);
}

void getActiveBranch(char* branch_path){
    FILE *head;

    // Open HEAD file and get branch path
    head = fopen(HEAD_PATH,"rb");
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

void getLastCommit(char* commitFatherHash){

    FILE *branch;
    char branch_path[PATHS_MAX_SIZE]; // It has '\n' at the end --> cropped

    getActiveBranch(branch_path);

    // Open active branch and get commit hash
    branch = fopen(branch_path,"rb");
    if (branch == NULL) perror("Error opening branch file: "), exit(EXIT_FAILURE);
    
    fgets(commitFatherHash,2*SHA_DIGEST_LENGTH+1,branch);
    // Correct hash (crop '\n' at the end)
    commitFatherHash[strcspn(commitFatherHash, "\n")] = 0;

    fclose(branch);
}

int buildCommitObject(char* commitMessage, char* commitTreeHash, char* user, char* commitHashOut){

    int fd_commit,findStatus=0;
    char prevCommit[2*SHA_DIGEST_LENGTH];
    char prevCommit_path[PATHS_MAX_SIZE];
    char prevCommitTreeHash[2*SHA_DIGEST_LENGTH];
    char prevCommitTree_path[PATHS_MAX_SIZE];

    // Check if nothing has changed (up to date)
    getLastCommit(prevCommit);
    
    // If not the first commit, search prev tree
    if (strcmp(prevCommit,"NONE") != 0)
    {
        findFile(OBJECTS_PATH, prevCommit,prevCommit_path,&findStatus);
        getFieldsFromCommit(prevCommit_path,prevCommitTreeHash,NULL,NULL);
        
        prevCommit_path[strcspn(commitTreeHash, "\n")] = 0;
        prevCommitTreeHash[strcspn(prevCommitTreeHash, "\n")] = 0;

        // If current CommitTreeHash and prev commitTreeHash are the same, return. Up to date
        if (strcmp(prevCommitTreeHash,commitTreeHash) == 0)
            return 0;
    }
    
    // Create temporal file for commit object content
    fd_commit = open(TEMP_COMMIT_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
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
    hashObject('c',TEMP_COMMIT_PATH,"",commitHashOut,"-w");

    // Remove temporal file
    if (remove(TEMP_COMMIT_PATH) == -1) perror("Error trying to remove './tempCommit': "),exit(1);
    return 1;
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
        
        if (write(fd_tree, "\n", strlen("\n")) < 1) perror("Error writing '\\n' into './tempTree': "),exit(1);
        if (write(fd_tree, strLevel, 3) < 1) perror("Error writing level into './tempTree': "),exit(1);
        
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
    if ((argc == 2) && (strcmp(argv[1],".")!=0)){
        printf("Error!\n");
        printf("Usage: './minimalAdd .' to add all files and folders.\n");
        exit(1);
    }

    int n_files = argc - 1;

    // Bad usage
    if (n_files != 1){
        printf("Error!\n");
        printf("Usage: './miniGitAdd .' to add all files and folders.\n");
        exit(1);
    }

    // Try to delete old index file
    if (remove("./.miniGit/index") == -1) perror("'./.miniGIT/index' not found. Will be created: ");

    // Add for all files
    if ((n_files == 1) && (strcmp(argv[1],".")==0)){
        addAllFiles(".","",0);
    }


}

void commit(int argc, char *argv[], char* username){

    /* 
    Commit function. 
    argv[0] --> .c file name
    argv[1] --> commit message. Use quotes to string-like message "This is a commit message"
    */

    // Bad usage
    if (argc != 2){
        printf("Error!\nUsage: './miniGitCommit \"COMMIT MESSAGE\"\n");
        exit(1);
    }

    char commitTreeHash[2*SHA_DIGEST_LENGTH], commitHash[2*SHA_DIGEST_LENGTH];
    char active_branch[PATHS_MAX_SIZE], branch_name[PATHS_MAX_SIZE];
    int ret;
    
    // Create commit tree object
    buildCommitTree(commitTreeHash);

    // Create commit object
    ret = buildCommitObject(argv[1],commitTreeHash,username,commitHash);
    if (ret == 0)
    {
        printf("Nothing to commit. Up to date\n");
        return;
    }

    // Update active branch
    getActiveBranch(active_branch);
    updateBranch(active_branch,commitHash);

    // Print succes messages
    getNameFromPath(active_branch,branch_name);
    printf("Active branch '%s' was updated with new commit!\n",branch_name);
}