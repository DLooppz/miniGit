#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<fcntl.h> 
#include<unistd.h>
#include<pthread.h>

void * clientThread(void *arg){
    printf("In thread\n");
    char message[1000];
    char buffer[1024];
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    
    // Create the socket. 
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);

    //Configure settings of the server address
    // Address family is Internet 
    serverAddr.sin_family = AF_INET;

    //Set port number, using htons function 
    serverAddr.sin_port = htons(7799);

    //Set IP address to localhost
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Connect the socket to the server using the address
    addr_size = sizeof serverAddr;
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
    strcpy(message,"Hello");
    printf("Client socket: %d\n",clientSocket);
    if( send(clientSocket , message , strlen(message) , 0) < 0){
        printf("Send failed\n");
    }

    //Read the message from the server into the buffer
    if(recv(clientSocket, buffer, 1024, 0) < 0){
        printf("Receive failed\n");
    }

    //Print the received message
    printf("Data received through: %s\n", buffer);
    usleep(1000);
    close(clientSocket);
    pthread_exit(NULL);
}

int main(){
    int i = 0;
    int err;
    pthread_t threadId[10];
    while(i<5){
        err = pthread_create(&threadId[i], NULL, &clientThread, NULL);
        // Check if thread is created sucessfuly
        if (err) {
            printf("Thread creation failed : %s\n", strerror(err));
            return err;
        }
        else
            printf("Thread Created with ID : 0x%lx\n", threadId[i]);
        i++;
        usleep(100);
    }
    sleep(2);
    i = 0;
    while(i< 5){
        pthread_join(threadId[i],NULL);
        printf("thread with id %ld joined\n",threadId[i]);
        i++;
    }
    return 0;
}