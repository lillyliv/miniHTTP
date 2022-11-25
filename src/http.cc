#include "http.hh"


#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include "../libs/jthread.hh"
#include <vector>
#include <assert.h>

bool shouldExit = false;

int server_fd, new_socket; long valread;
struct sockaddr_in address;
int addrlen;

// https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**)malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void connection(int mySocket) {
    std::cout << "New connection" << std::endl;
    FILE* responeFile;

    char buffer[30000] = {0};
    valread = read(mySocket, buffer, 30000);

    // printf("%s\n",buffer );

    char* path = str_split(str_split(buffer, '\n')[0], ' ')[1];

    if(strstr(path, "..")) {
        write(mySocket, "Going up a directory not allowed!", 33);
        close(mySocket);
        return;
    }

    if(strcmp(path, "/") == 0) {
        std::cout << "requesting index.html" << std::endl;
        responeFile = fopen("index.html", "rb");


    } else {
        memmove(path, path+1, strlen(path));
        responeFile = fopen(path, "rb");

        std::cout << path << std::endl;
    }

    if (NULL == responeFile) {
            write(mySocket, "the requested file could not be found!", 38);
            close(mySocket);
            return;
        }

        char* filebuf = 0;
        long length;

        fseek (responeFile, 0, SEEK_END);
        length = ftell (responeFile);
        fseek (responeFile, 0, SEEK_SET);
        filebuf = (char *)malloc (length);
        if (filebuf)
        {
            fread (filebuf, 1, length, responeFile);
        }
        fclose (responeFile);

        std::cout << "index.html" << std::endl;

        write(mySocket, filebuf, length);

        close(mySocket);
        return;
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

    std::cout << "Server now listening" << std::endl;

    while(!shouldExit) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        nonstd::jthread connectionThread(connection, new_socket);
        connectionThread.detach();

    }
}