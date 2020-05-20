#ifndef MINIGITUTILS_H_   /* Include guard */
#define MINIGITUTILS_H
#include <stdio.h>
#include <openssl/sha.h>

#define INDEX_MAX_ENTRIES 250
#define MAX_FILENAME 60

// --------------------------------------------------------------------
// index (key-value table) 
typedef struct DirIndex{
    char SHAObjectsList[INDEX_MAX_ENTRIES][2*SHA_DIGEST_LENGTH];  /* 7a7a472abf3dd9643fd615f6da379c4acb3e3a */
    char FilesList[INDEX_MAX_ENTRIES][MAX_FILENAME]; /* NameofFileOrFolder */
}Index_t;

// --------------------------------------------------------------------
// miniGitObjects: BlobObject_t (Blobs), TreeObject_t (trees), CommitObject_t (commits) 
typedef struct{
    char objectType[10];
    char contentSize[10];
    char *content;
    char nameSHA1[2*SHA_DIGEST_LENGTH]; /* In HEX */
}GenericObject_t;

typedef struct{
    GenericObject_t obj;
}BlobObject_t;

typedef struct tree{
    GenericObject_t obj;
    struct tree *subdirs;
    int n_subdirs;
}TreeObject_t;

typedef struct Commits{
    GenericObject_t obj;
    struct Commits *fatherCommit;
    TreeObject_t *tree;
}CommitObject_t;

// --------------------------------------------------------------------
// refs/heads (branchs locales)
typedef struct{
    char name[MAX_FILENAME];
    CommitObject_t* commit;
}Branch;

// HEAD
typedef struct{
    Branch* ref_ref_head; 
}HEAD;

// --------------------------------------------------------------------
// Methods (just for C objects - dont create nor modify files)
int _indexAddEntrie(char* SHAObject, char* file);
int _indexRemoveEntrie(Index_t *index,char* SHAObject);

void _genericObjectCat(GenericObject_t* obj, char catType, char* str_out); /* 't': type ; 'c': content ; 's': size*/
void _genericObjectSHA1(GenericObject_t* obj, char* SHA1_out);
int _genericObjectCheckExistence(char* SHA1_name);

BlobObject_t* _blobObjectCreateFromFile(FILE* file);
BlobObject_t* _blobObjectCreateFromString(char* str);
void _blobObjectDestroy(BlobObject_t* blob);

TreeObject_t* _treeObjectCommitCreate(Index_t index_updated, TreeObject_t *root_dirs);
TreeObject_t* _treeObjectDirCreate(int subdirs);
void _treeObjectAddEntrie(char* filename, char* SHA1name);
void _treeObjectSetName(TreeObject_t* finished_tree); /* Update SHA1 name when had all of its entries */
void _treeObjectDestroy(TreeObject_t* tree);
void _treeObjectPrintEntries(TreeObject_t* tree);
int _treeObjectGetTotalNumberOfSubdirs(TreeObject_t* tree);

CommitObject_t* _commitObjectCreate(HEAD head, TreeObject_t* commitTree);
void _commitObjectDestroy(CommitObject_t* commit);

Branch* _branchCreate(CommitObject_t* snapshot,char* name);
void _branchDestroy(Branch* branch);
void _branchUpdate(Branch* branch, CommitObject_t* currentSnapshot);

void _HEADUpdate(HEAD* head, Branch* branch);

// --------------------------------------------------------------------
// Functions

int setIndexFile(Index_t* index);
void readIndexFile(char* content);
void printIndexFile(char* str_out); /* optional save output */

int createObjectFile(GenericObject_t* obj);
int removeObjectFile(GenericObject_t* objToRemove, FILE* objFileToRemove);
int openObjectFile(char* SHA1name, FILE* objFile);
int closeObjectFile(FILE* objFile);
void catObjectFile(FILE* objFile, char catType, char* str_out); /* optional save output */
void checkSumSHA1ObjectFile(FILE* objFile, char* SHA1_out);

int createBranchFile(Branch* branch);
int removeBranchFile(Branch* branchToRemove);
int restoreVersion(Branch* branch, CommitObject_t* oldVersion); /* Also acts on branch file */
int checkoutBranch(HEAD* headToUpdate, Branch* BranchToCheckout); /* Also acts on HEAD file */

// --------------------------------------------------------------------
// Setup functions 
int createGitDir();

int clientSetUp();
int serverSetup();

int setupIndex();
int setupObjects();
int setupBranchs();
int setupHEAD();

// --------------------------------------------------------------------
// User functions 

void init();
void add(char* arg);
void commit(char* mssg); /* agregar lo del mssg*/
void push();
void pull();
void restore(char* SHA1version);
void cat_file(char* SHA1File, char* cat_type); /* "-p": content; "-t: type" */
void hash_object(char* fileName, char* optionalArgs); /* -w: add object */
void checkout(char* branchName);
void log(char* SHA1Commit); /* By default, master */ 
void exit();

#endif 