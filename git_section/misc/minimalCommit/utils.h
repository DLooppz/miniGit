#include <stdio.h>

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
void add(int argc, char *argv[]);
void commit(int argc, char *argv[], char* username);
