#ifndef MINIGITUTILS_H_   /* Include guard */
#define MINIGITUTILS_H

#include <stdio.h>
#define TREE_MAX_SIZE 14000
#define PATHS_MAX_SIZE 512
#define INDEX_PATH "./.miniGit/index"
#define HEAD_PATH "./.miniGit/HEAD"
#define OBJECTS_PATH "./.miniGit/objects/"
#define TEMP_COMMIT_TREE_PATH "./.miniGit/tempCommitTree"
#define TEMP_COMMIT_PATH "./.miniGit/tempCommit"

// -----------------------------------------------------------------------------------------------
// Backend functions
void findFile(char *basePath,char* hashToFind, char* pathFound, int* findStatus);
void computeSHA1(char* text, char* hash_output);
void addObjectFile(char* hashName, char* contentPlusHeader);
void buildHeader(char type, long contentSize, char* header_out);
void hashObject(char type, char* path, char* fileName, char* hashName_out, char* creationFlag);
int getFieldsFromIndex(char* index_line, char* hash_out, char* path_out);
void getFieldsFromCommit(char* commitPath, char* tree, char* prevCommit, char* user);
int getFileLevel(char* path);
void getNameFromPath(char* path, char* name);
void buildCommitTree(char* commitTreeHash);
void getActiveBranch(char* branch_path);
void setActiveBranch(char* branchName); // TODO
void updateBranch(char* branchPath, char* newCommitHash);
void getCommitFather(char* commitFatherHash);
int buildCommitObject(char* commitMessage, char* commitTreeHash, char* user, char* commitHashOut);
void updateIndex(char* hash, char* path, char type);
void addAllFiles(char* basePath,char* prevFolder, int level);

// -----------------------------------------------------------------------------------------------
// User functions 
void add(int argc, char *argv[]);
void commit(int argc, char *argv[], char* username);
void cat_file(char* SHA1File, char* cat_type); /* "-p": content; "-t: type" */ // EASY TODO
void hash_object(char* fileName, char* optionalArgs); /* -w: add object */ // EASY TODO

void init(); // TODO
void push(); // TO MERGE WITH K
void pull(); // TO MERGE WITH K
void checkout(char* branchname); // TODO
void checkoutCommit(char* SHA1Commit); // TODO
void logHist(char* SHA1Commit); /* By default, master */ //TODO
void exit(); // TO MERGE WITH K

#endif 
