// Run with:
// clear && gcc one_client.c -o one_client && ./one_client

#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<fcntl.h> 
#include<unistd.h>

#define SIZE 1024
#define PORT 7000

void error(const char *msg) {
    perror(msg);
    exit(0);
}

typedef struct {
    int sock_fd;
    struct sockaddr_in sock_addr;
} socketInfo_t;

int main(){
    int i = 0;
    int opt = 1;
    int n_bytes, port_number;
    char message[SIZE + 1];
    char buffer[SIZE + 1];
    int commSocket, dataSocket;
    socketInfo_t commSock_info, dataSock_info;
    socklen_t commAddr_size, dataAddr_size;
    
    // Create commands socket
    commSock_info.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    commSocket = commSock_info.sock_fd;
    // Configure commands socket
    setsockopt(commSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));
    commSock_info.sock_addr.sin_family = AF_INET;
    commSock_info.sock_addr.sin_port = htons(PORT);
    commSock_info.sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bzero(commSock_info.sock_addr.sin_zero, sizeof commSock_info.sock_addr.sin_zero);

    // Connect the socket to the server using the address
    commAddr_size = sizeof(commSock_info.sock_addr);
    connect(commSocket, (struct sockaddr *) &commSock_info.sock_addr, commAddr_size);

    // Read the welcome message from server (which is the port to be used for the data socket)
    n_bytes = recv(commSocket, buffer, SIZE, 0);
    buffer[n_bytes] = '\0';
    if(n_bytes < 0)
        error("ERROR Receive failed");

    // Create data socket
    dataSock_info.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    dataSocket = dataSock_info.sock_fd;
    // Configure data socket
    setsockopt(commSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));
    dataSock_info.sock_addr.sin_family = AF_INET;
    dataSock_info.sock_addr.sin_port = htons(atoi(buffer));
    dataSock_info.sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bzero(commSock_info.sock_addr.sin_zero, sizeof commSock_info.sock_addr.sin_zero);
    dataAddr_size = sizeof(dataSock_info.sock_addr);
    connect(dataSocket, (struct sockaddr *) &dataSock_info.sock_addr, dataAddr_size);

    while(1){
        bzero(buffer, sizeof(buffer));
        bzero(message, sizeof(message));

        printf("Enter 'comm' to use commands socket or 'data' to use data socket.\nThen hit enter.\n");
        scanf("%s", message);
        
        if(strcmp(message,"comm") == 0){
            printf("(comm)Enter mssg: ");
            scanf("%s", message);
            strcat(message,"\0");

            if( send(commSocket , message , strlen(message) , 0) < 0){
                error("ERROR (comm) Send failed");
            }

            // Read the message from the server into the buffer
            n_bytes = recv(commSocket, buffer, SIZE, 0);
            buffer[n_bytes] = '\0';
            if(n_bytes < 0)
                error("ERROR (comm) Receive failed");

            // Print the received message
            printf("(comm)Msg received: %s\n", buffer);

            if(strcmp(message,"stop") == 0)
                break;
        } else if (strcmp(message,"data")==0){
            printf("(data)Enter mssg: ");
            scanf("%s", message);
            strcat(message,"\0");

            if( send(dataSocket , message , strlen(message) , 0) < 0){
                error("ERROR (data) Send failed");
            }

            // Read the message from the server into the buffer
            n_bytes = recv(dataSocket, buffer, SIZE, 0);
            buffer[n_bytes] = '\0';
            if(n_bytes < 0)
                error("ERROR (data) Receive failed");

            // Print the received message
            printf("(data)Msg received: %s\n", buffer);

            if(strcmp(message,"stop") == 0)
                break;
        }
        printf("'comm' or 'data' are the only valid inputs. Try again.\n\n");
    }
    close(commSocket);
    close(dataSocket);
    return 0;
}