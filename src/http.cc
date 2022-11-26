#include "http.hh"

#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include "../libs/jthread.hh"
#include "../libs/json.hh"
#include "util.hh"

using json = nlohmann::json;

bool shouldExit = false;

int server_fd, new_socket; long valread;
struct sockaddr_in address;
int addrlen;
json serverPrefrences;

const char ErrorPage404[1024] = "<html><body><center><h1>Error 404 page not found!</h1><footer>This message was sent by MiniHTTP (https://github.com/lillyliv/miniHTTP)</footer></center></body></html>";
const char* Error500 = "<html><body><center><h1>Error 500 internal server error!</h1><footer>This message was sent by MiniHTTP (https://github.com/lillyliv/miniHTTP)</footer></center></body></html>";
const char* Error418 = "Error I'm a teapot!"; // only use if server is running on a teapot instead of a coffee pot

char* constructHTTPHeader500(char* server, long long length) {
    char* HTTPHeader = (char*) malloc(strlen(server)+1000);
    sprintf(HTTPHeader, "HTTP/1.1 500 Internal Server Error\nServer: %s\nContent-Type: text/html\nContent-Length: %lld\nAccept-Ranges: bytes\nConnection:close\n\n", server, length);

    return HTTPHeader;
}

char* constructHTTPHeader200(char* server, long long length) {
    char* HTTPHeader = (char*) malloc(strlen(server) + 1000);
    sprintf(HTTPHeader, "HTTP/1.1 200 OK\nServer: %s\nContent-Type: text/html\nContent-Length: %lld\nAccept-Ranges: bytes\nConnection: close\n\n", server, length);

    return HTTPHeader;
}
char* constructHTTPHeader403(char* server, long long length) {
    char* HTTPHeader = (char*) malloc(strlen(server) + 1000);
    sprintf(HTTPHeader, "HTTP/1.1 403 Forbidden\nStatus:403 Forbidden\nServer: %s\nContent-Type: text/html\nContent-Length: %lld\nAccept-Ranges: bytes\nConnection: close\n\n", server, length);

    return HTTPHeader;
}
char* constructHTTPHeader404(char* server, long long length) {
    char* HTTPHeader = (char*) malloc(strlen(server) + 1000);
    sprintf(HTTPHeader, "HTTP/1.1 404 Not Found\nStatus: 404 Not Found\nServer: %s\nContent-Type: text/html\nContent-Length: %lld\nAccept-Ranges: bytes\nConnection: close\n\n", server, length);

    return HTTPHeader;
}
char* constructHTTPHeader418(char* server, long long length) {
    char* HTTPHeader = (char*) malloc(1000);
    sprintf(HTTPHeader, "HTTP/1.1 418 I'm a teapot\nStatus: 418 Im a teapot\nServer: %s\nContent-Type: text/html\nContent-Length: %lld\nAccept-Ranges: bytes\nConnection: close\n\n", server, length);

    return HTTPHeader;
}

char* HTTPResponseBuilder(int responseCode, char* response) {
    char* servstring = (char*)serverPrefrences["version"].get_ref<const std::string&>().c_str();
    char* datatoreturn;
    char* headertoreturn;

    if (responseCode == 200) {
        headertoreturn = constructHTTPHeader200(servstring, strlen(response));
        datatoreturn = (char*) malloc (strlen(headertoreturn) + strlen(response));
        sprintf(datatoreturn, "%s%s", headertoreturn, response);
    } else if (responseCode == 404) {
        headertoreturn = constructHTTPHeader404(servstring, strlen(ErrorPage404));
        datatoreturn = (char*)malloc(strlen(headertoreturn) + strlen(ErrorPage404));
        sprintf(datatoreturn, "%s%s", headertoreturn, ErrorPage404);
    } else if (responseCode == 418) {
        headertoreturn = constructHTTPHeader418(servstring, strlen(Error418));
        datatoreturn = (char*) malloc(strlen(headertoreturn) + strlen(Error418));
        sprintf(datatoreturn, "%s%s", headertoreturn, Error418);
    } else {
        headertoreturn = constructHTTPHeader500(servstring, strlen(Error500));
        datatoreturn = (char*)malloc(strlen(headertoreturn) + strlen(Error500));
        sprintf(datatoreturn, "%s%s", headertoreturn, Error500);
    }

    free(headertoreturn);

    return datatoreturn;
}


void connection(int mySocket) {
    std::cout << "New connection" << std::endl;
    char* versionString = (char*)serverPrefrences["version"].get_ref<const std::string&>().c_str();

    FILE* responeFile;
    char* response;

    char buffer[30000] = {0};
    valread = read(mySocket, buffer, 30000);

    char* path = str_split(str_split(buffer, '\n')[0], ' ')[1];

    if (strstr(path, "..")) {
        response = HTTPResponseBuilder(404, NULL);
        write(mySocket, response, strlen(response));
        close(mySocket);
        free(response);
        return;
    }

    if (strcmp(path, "/") == 0) {
        std::cout << "requesting index.html" << std::endl;
        responeFile = fopen("index.html", "rb");

    } else {
        memmove(path, path+1, strlen(path));
        responeFile = fopen(path, "rb");

        std::cout << path << std::endl;
    }

    if (NULL == responeFile) {
        response = HTTPResponseBuilder(404, NULL);
        write(mySocket, response, strlen(response));
        close(mySocket);
    } else {

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

        response = HTTPResponseBuilder(200, filebuf);
        write(mySocket, response, strlen(response));
        close(mySocket);
    }

    free(response);

    return;
}

void startServer(char* pathToJson) {
    shouldExit = false;

    FILE *fp;

    fp = fopen(pathToJson, "r");
    serverPrefrences = json::parse(fp);

    int port = serverPrefrences["port"];

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

        if (serverPrefrences["useThreadsForConnection"]) {
            nonstd::jthread connectionThread(connection, new_socket);
            connectionThread.detach();
        } else {
            connection(new_socket);
        }
    }
}