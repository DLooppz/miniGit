#include<stdlib.h>
#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<errno.h>
#include<stdbool.h>
#include<string.h>
#include<dirent.h>
#include<sys/stat.h>
#include<termios.h>
#include<unistd.h>
#include<fcntl.h>
#include<assert.h> 

#define USERLEN 64


int clean_directory(const char *path, const char *exclude, char* miniGitPath) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;
    if (d) {
        struct dirent *p;

        r = 0;
        while (!r && (p=readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..") || !strcmp(p->d_name, "NoBorrar"))
                continue;

            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = clean_directory(buf, exclude, miniGitPath);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r && strcmp(path,exclude)!=0 && strcmp(path,miniGitPath) != 0)
        r = rmdir(path);

    return r;
}

int main(void){

    clean_directory("./Borrar","./Borrar","./Borrar/NoBorrar");
    return 0;
}