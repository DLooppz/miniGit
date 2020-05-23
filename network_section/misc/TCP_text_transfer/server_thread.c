#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

char buffer[256];
int newsockfd, n;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void *threadFunc(void * arg){
     bzero(buffer, 256);
     n = read(newsockfd, buffer, 255);
     if (n < 0)
          error("ERROR reading from socket");
     printf("Here is the message: %s\n", buffer);
     n = write(newsockfd, "I got your message", 18);
     if (n < 0)
          error("ERROR writing to socket");
     close(newsockfd);
     return NULL;
}

int main(int argc, char *argv[]) {
     //Variable declarations
     pthread_t threadId;
     int context, err, pid;
     int sockfd, portno;
     socklen_t clilen;
     int val = 1;
     struct sockaddr_in serv_addr, cli_addr;
     clilen = sizeof(cli_addr);

     // Check if port was provided as an argument when executing
     if (argc < 2) {
          fprintf(stderr, "ERROR, no port provided\n");
          exit(1);
     }
     portno = atoi(argv[1]);
     
     // Set the atributes of serv_addr, which is a struct sockaddr_in
     bzero((char *) &serv_addr, sizeof(serv_addr));
     serv_addr.sin_family = PF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     // Socket function, which creates and endpoint for communincation and returns the file descriptor associated
     sockfd = socket(PF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
          error("ERROR opening socket");
     
     // Set socket options, SOL_SOCKET means that the socket API is gonna be used. And SO_REUSEADDR prevents errors such as "Error on binding, address already in use" when the program is executed twice
     setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

     // When  a  socket is created with socket(2), it exists in a name space (address family) but has no address assigned to it.  bind() assigns the address specified by addr to the socket referred to by the file descriptor sockfd.
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
          error("ERROR on binding");
     
     // Marks the socket as a passive socket
     listen(sockfd, 5);
     
     // Start communications loop
     while (1) {
          // The  accept() system call is used with connection-based socket types (SOCK_STREAM, SOCK_SEQPACKET).  It extracts the first connection request on the queue of pending connections for the listening socket, sockfd, creates a new connected socket, and returns a new file descriptor referring  to  that socket.  The newly created socket is not in the listening state.  The original socket sockfd is unaffected by this call.
          newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
          if (newsockfd < 0)
               error("ERROR on accept");
          err = pthread_create(&threadId, NULL, &threadFunc, &context);
     }
     return 0;
}