#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>

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

void addObjectFile();

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

void hashObject(char type, char* path, char* hashName_out, char* creationFlag){
    /* 
    char type: "b":blob ; "c":commit ; "t":tree
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
    printf("\n\nTamanio %ld\n",strlen(content));
    printf("Content+Header: l%sl\n\n",contentPlusHeader);

    // Check if object should be created
    if (strcmp(creationFlag,"-w") == 0){
        printf("TODO: create object as requested! jejeje\n");
    }

    // Close and free 
    fclose(original_file);
    free(content);
    free(contentPlusHeader);
}


int main(int argc, char *argv[]){

    // Correct usage
    if (argc < 2){
        printf("Error!\nUsage: ./minimalAdd [File1] [File2]... [FileN]\n");
        printf("Usage: ./minimalAdd .\n");
        exit(1);
    }

    // "." to add all files
    int n_files = argc - 1;
    if ((n_files == 1) && (strcmp(argv[1],".")==0)){
        n_files = -1;
        printf("All files to add!\n");
    }
    // [files] in other case TODO
    else{
        printf("Files to add: ");
        for (int i=0;i<n_files;i++){
            printf("%d)'%s' ",i+1,argv[i+1]);
        }
        printf("\n");
    }

    char hashedFile[2*SHA_DIGEST_LENGTH];
    hashObject('b',argv[1],hashedFile,"-w");
    printf("Hash of file: %s\n", hashedFile);

    return 0;
}