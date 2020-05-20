#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

int main(void){

    FILE *original_file, *hashed_file;
    long lSize;
    char *buffer;
    unsigned char hashed_text[SHA_DIGEST_LENGTH];

    // Open files (b: security for non Linux env)
    original_file = fopen ("ToBeHashed.txt" , "rb");
    if(!original_file) perror("Error opening in file to be hashed"),exit(1);

    hashed_file = fopen("Hashed.txt", "wb");
    if(!hashed_file) perror("Error opening file hashed\n"),exit(2);

    // Set the file position indicator at the end
    fseek(original_file , 0L , SEEK_END);

    // Get the file position indicator 
    lSize = ftell(original_file);
    rewind(original_file);

    // allocate memory for entire content
    buffer = calloc(1, lSize + 1);
    if(!buffer) fclose(original_file),fputs("memory alloc fails",stderr),exit(1);

    // copy the file into the buffer 
    if(fread( buffer , lSize, 1 , original_file) != 1)
        fclose(original_file),free(buffer),fputs("entire read fails",stderr),exit(1);

    // Now buffer is a string containing the whole text
    printf("********* Original file:\n%s\n",buffer);

    // Hash the file 
    SHA1(buffer,sizeof(buffer),hashed_text);
    printf("********** Hashed file:\n%s\n",hashed_text);

    // Convert to hex
    char hex_hashed_text[2*SHA_DIGEST_LENGTH] = {0,};
    for(int i=0;i<SHA_DIGEST_LENGTH;i++){
        sprintf(hex_hashed_text + i * 2, "%02x", hashed_text[i]); // <-- each 2 bytes. e.g: 1 = 01, 255 = FF
    }
    printf("********** Hashed file in hex:\n%s\n", hex_hashed_text);
    fprintf(hashed_file, "%s", hex_hashed_text);

    // Close and free 
    fclose(original_file);
    fclose(hashed_file);
    free(buffer);

    return 0; 
}
