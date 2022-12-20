#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define PORT 8080 // the port users will be connecting to

#define BACKLOG 10 // how many pending connections queue will hold
char *ok = "HTTP/1.1 200 OK\r\n";
char *not_found = "HTTP/1.1 404 Not Found\r\n";

typedef enum RequestType {
    GET,
    POST,
    WrongReq
} RequestType;

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

enum RequestType parseRequest(char *request)
{
    if (strstr(request, "GET") != NULL)
        return GET;
    else if (strstr(request, "POST") != NULL)
        return POST;
    else
        return WrongReq;
}

char *getFileName(char *request)
{
    char *fileName = (char *) malloc(100);
    char *token = strtok(request, " ");
    token = strtok(NULL, " ");
    strcpy(fileName, token);
    return fileName;
}

char *readFile(char *fileName)
{
    FILE *fp;
    char *content = (char *) malloc(10000);
    fp = fopen(fileName, "r");
    if (fp == NULL)
        return NULL;
    char c = fgetc(fp);
    int i = 0;
    while (c != EOF) {
        content[i] = c;
        c = fgetc(fp);
        i++;
    }
    content[i] = '\0';
    fclose(fp);
    return content;
}

void sendResponse(int client_fd, char *response)
{
    send(client_fd, response, strlen(response), 0);
}

void handleGetRequest(int client_fd, char *request)
{
    char *fileName = getFileName(request);
    char *content = readFile(fileName);
    if (content == NULL)
        sendResponse(client_fd, not_found);
    else {
        sendResponse(client_fd, ok);
        sendResponse(client_fd, content);
    }
}

void handlePostRequest(int client_fd, char *request)
{
    sendResponse(client_fd, ok);
    int valread;
    char buffer[1024] = {0};
    valread = read(client_fd, buffer, 1024);
    printf("server: message is %s\n", buffer);
}

/***
this function will be called to handle each client
 I should mske local variables for each thread avoid getting the wrong value
 from the global variables
***/
void *handleClient(void *new_fd)
{
    // get the argument from the thread
    int client_fd = *(int *)new_fd;
    int valread;
    char buffer[1024] = {0};

    valread = read(client_fd, buffer, 1024);
    RequestType requestType = parseRequest(buffer);
    printf("server: message is %s\n", buffer);
    printf("request type: %d\n", parseRequest(buffer));

    if (requestType == GET) {
        printf("GET request\n");
        handleGetRequest(client_fd, buffer);
    }
    else if (requestType == POST) {
        printf("POST request\n");
        handlePostRequest(client_fd, buffer);
    }
    else {
        printf("Invalid request\n");
        sendResponse(client_fd, not_found);
    }

    // here I should wait time out till reciving an other request or close the connection
    close(client_fd);
    return NULL;
}

int main(int argc, char const *argv[])
{
    // listen on server_fd, new connection on new_fd
    int server_fd, new_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    struct sigaction sa;

    pthread_t thread_id;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket Creation Failed!");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // binds the socket to the address and port number specified in addr
    // bind the server to the localhost, hence we use INADDR_ANY to specify the IP address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("request is refused");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("server: waiting for connections...\n");
    while (1) {
        // Accept and delegate client conncection to worker thread
        if ((new_fd = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {

            perror("accept client conncetion has failed");
            exit(EXIT_FAILURE);
        }

        // TODO
        // parsing command and specify if it GET or POST ------------ DONE
        // if target file does not exists return error ------------------
        // transmit content of file (reads from the file and write on the socket) - in case of GET
        // wait for new requests (presistent connection)
        // close if connection timed out

        printf("server: got connection\n");

        pthread_t thread;
        int threadid = pthread_create(&thread, NULL, &handleClient, &new_fd);
        if (threadid < 0) {
            perror("thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}