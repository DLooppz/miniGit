#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    //Variable declarations
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[100];

    // Check if hostname of server and port were provided as an argument when executing
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    // Set the atributes of serv_addr, which is a struct sockaddr_in
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);

     // Socket function, which creates and endpoint for communincation and returns the file descriptor associated
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    // Connect to the socket
    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    // File to be sent
    FILE *fp;
    fp = fopen("file1.txt","r");
    if(fp == NULL)
        error("ERROR in opening File");

    // Send file
    while(fgets(buffer, 100, fp) != NULL){
        n = write(sockfd, buffer, strlen(buffer));
        if (n<0)
            error("ERROR writing to socket");
    }
    fclose(fp);
    printf("1aaa\n");

    // Clear buffer that was used to send the message. Then read message and place it on buffer
    memset(buffer, 0, sizeof(buffer));
    n = read(sockfd,buffer,100);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);

    close(sockfd);

    return 0;
}
