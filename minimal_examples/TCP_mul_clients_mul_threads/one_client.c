#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<fcntl.h> 
#include<unistd.h>

#define SIZE 1024

int main(){
    int i = 0;
    int opt = 1;
    int n_bytes, port_number;
    char message[SIZE + 1];
    char buffer[SIZE + 1];
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    
    // Create the socket. 
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));

    //Configure settings of the server address
    // Address family is Internet 
    serverAddr.sin_family = AF_INET;

    //Set port number, using htons function 
    scanf("%d",&port_number);
    serverAddr.sin_port = htons(port_number);

    //Set IP address to localhost
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Connect the socket to the server using the address
    addr_size = sizeof serverAddr;
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

    while(1){
        printf("Enter command: ");
        scanf("%s", message);

        strcat(message,"\0");

        if( send(clientSocket , message , strlen(message) , 0) < 0){
            printf("Send failed\n");
        }

        //Read the message from the server into the buffer
        n_bytes = recv(clientSocket, buffer, 1024, 0);
        buffer[n_bytes] = '\0';
        if(n_bytes < 0){
            printf("Receive failed\n");
        }

        //Print the received message
        printf("Data received: %s\n", buffer);

        if(strcmp(message,"stop") == 0)
            break;

        bzero(buffer, sizeof(buffer));
        bzero(message, sizeof(message));
    }
    close(clientSocket); 
    return 0;
}