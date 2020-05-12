#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    int val = 1;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
         fprintf(stderr, "ERROR, no port provided\n");
         exit(1);
    }
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
         error("ERROR opening socket");
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = PF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    int pid;
    while (1) {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         if (newsockfd < 0)
                error("ERROR on accept");
         //fork new process
         pid = fork();
         if (pid < 0) {
              error("ERROR in new process creation");
         }
         if (pid == 0) {
            //child process
            //close(sockfd);
            //do whatever you want
            bzero(buffer, 256);
            n = read(newsockfd, buffer, 255);
            if (n < 0)
                error("ERROR reading from socket");
            printf("Here is the message: %s\n", buffer);
            n = write(newsockfd, "I got your message", 18);
            if (n < 0)
                 error("ERROR writing to socket");
            close(newsockfd);
          } else {
             //parent process
             close(newsockfd);
          }
    }
   return 0;
}