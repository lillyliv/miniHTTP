#include "http.hh"


#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include "../libs/jthread.hh"
#include <vector>

bool shouldExit = false;

int server_fd, new_socket; long valread;
struct sockaddr_in address;
int addrlen;
char* hello = "Hello from server";

void connection() {
    const int mySocket = new_socket;
    char buffer[30000] = {0};
    valread = read(mySocket , buffer, 30000);
    printf("%s\n",buffer );
    write(mySocket , hello , strlen(hello));
    std::cout << "New connection" << std::endl;
    close(mySocket);
}

void startServer(int port) {
    shouldExit = false;
    std::cout << "starting server on port " << port << std::endl;

    addrlen = sizeof(address);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );
    
    memset(address.sin_zero, '\0', sizeof address.sin_zero);
    
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }

    while(!shouldExit) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        // std::thread newConnection (connection);
        // clientThreads[currentIndex].detach();

        nonstd::jthread connectionThread(connection);
        connectionThread.detach();

    }
}