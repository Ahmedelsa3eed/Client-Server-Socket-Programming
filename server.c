#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>

#define PORT 8080 // the port users will be connecting to
#define MaxClients 10
#define MAXDATASIZE 1024 // 4k bytes: max number of bytes we can get at once
#define BACKLOG 10 // how many pending connections queue will hold
#define MAX_TIMEOUT 10 // 10 seconds

char *ok = "HTTP/1.1 200 OK\r\n";
char *not_found = "HTTP/1.1 404 Not Found\r\n";

typedef enum RequestType {
    GET,
    POST,
    WrongReq
} RequestType;

typedef struct timeOutCalc {
    clock_t start;
    int loc;
    int maxTime;
    int new_fd;
} timeOutCalc;

pthread_mutex_t lock;
int numClients = 0;
int clientSockets[MaxClients];
double connectionTimeOut;


void sigchld_handler(int s);

enum RequestType parseRequest(char *request);

char *getFileName(char *request);

char *readFile(char *fileName);

void sendResponse(int client_fd, char *response);

void handleGetRequest(int client_fd, char *request);
void saveFile(char *filePath, size_t nBytes, char *buffer);

void handlePostRequest(int client_fd, char *request);

void *calculateTimeOut(void *arg);


void *handleClient(void *new_fd) ;


// function to remove the socket descriptor
void removeSocket(int socket);

void updateTimeOut();



int main(int argc, char const *argv[])
{
    /// listen on server_fd, new connection on new_fd
    signal(SIGPIPE, SIG_IGN);
    int server_fd, new_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct sigaction sa;

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
        /// Accept and delegate client conncection to worker thread
        if ((new_fd = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {

            perror("accept client conncetion has failed");
            exit(EXIT_FAILURE);
        }

        printf("server: got connection\n");
        pthread_mutex_lock(&lock);

        if (numClients < MaxClients) {
            clientSockets[numClients] = new_fd;
            numClients++;
            updateTimeOut();
            pthread_mutex_unlock(&lock);
            pthread_t thread;
            pthread_create(&thread, NULL, handleClient, (void *) &new_fd);
            if (thread < 0) {
                perror("thread creation failed");
                exit(EXIT_FAILURE);
            }
        } else {
            pthread_mutex_unlock(&lock);
            printf("server: too many clients\n");
            close(new_fd);
        }



    }

}




void sigchld_handler(int s)
{
    /// waitpid() might overwrite errno, so we save and restore it:
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
    /// add . to the file name
    char *temp = (char *) malloc(100);
    strcpy(temp, ".");
    strcat(temp, fileName);
    free(fileName);
    return temp;
}

char *readFile(char *fileName)
{
    FILE *fp;
    char *content = (char *) malloc(10000);
    fp = fopen(fileName, "rb");
    if (fp == NULL)
        return NULL;
    unsigned int nBytes = 0;
    while(fread(&content[nBytes], sizeof(char), 1, fp) == 1) {
        nBytes++;
    }
    fclose(fp);
    return content;
}

void sendResponse(int client_fd, char *response)
{
    send(client_fd, response, strlen(response), MSG_NOSIGNAL);
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
void saveFile(char *filePath, size_t nBytes, char *buffer)
{
    char *newfilePath = (char *) malloc(50);
    strcat(newfilePath, "serverPostFiles/");
    strcat(newfilePath, basename(filePath));

    FILE *fptr;
    if ((fptr = fopen(newfilePath, "wb")) == NULL) {
        fprintf(stderr, "Couldn't open file for writing. \n");
        return;
    }
    int toBeWritten = 0;
    while (toBeWritten < nBytes) {
        fwrite(&buffer[toBeWritten], sizeof(char), 1, fptr);
        toBeWritten++;
    }
    printf("Received file saved in: %s\n", newfilePath);
    free(newfilePath);
    fclose(fptr);
}

void handlePostRequest(int client_fd, char *request)
{
    sendResponse(client_fd, ok);
    int valread;
    char buffer[1024] = {0};
    valread = read(client_fd, buffer, 1024);
    saveFile( "ay haga", valread, buffer);
}

void removeSocket(int socket) {
    int i = 0;
    for (i = 0; i < MaxClients; i++) {
        // if position is empty
        if (clientSockets[i] == socket) {
            clientSockets[i] = 0;
            break;
        }
    }
    // shift all the elements to the left
    for (i = 0; i < MaxClients - 1; i++) {
        if (clientSockets[i] == 0) {
            clientSockets[i] = clientSockets[i + 1];
            clientSockets[i + 1] = 0;
        }
    }
}
void *calculateTimeOut(void *arg)
{
    timeOutCalc *timeOut = (timeOutCalc *) arg;
    printf( "I am in the time out thread");

    while ( (double)( (clock_t) clock() - timeOut->start)/ CLOCKS_PER_SEC < timeOut->maxTime ) {/// wait for x seconds to get the request from the client

        if (timeOut->loc == 1) { /// if the request is received
            printf("server receive request from client %d\n",  timeOut->new_fd);
            printf("server received request at %ld\n", clock());

            while (timeOut->loc == 1) {
                /// wait for the client to send the request
            }
            printf("Waited for 10 seconds, closing the connection\n");
            printf("new start time is %ld\n", timeOut->start);
            /// start get updated in the client thread and the same for the loc
        }
    }

    printf("Waited for %d seconds, closing the connection\n", timeOut->maxTime);
    timeOut->start = -1;/// reset the start time to -1 to indicate that the connection is closed

    pthread_mutex_lock(&lock);
    removeSocket(timeOut->new_fd);
    numClients--;
    pthread_mutex_unlock(&lock);

    close( timeOut->new_fd);         /// close the connection

    pthread_exit(NULL);        /// exit the thread
}
void updateTimeOut(){
    connectionTimeOut = numClients? MAX_TIMEOUT/numClients : MAX_TIMEOUT;
    printf("Currently handeling %d connections\nTimeout is %f seconds\n",numClients, connectionTimeOut);
}
/***
this function will be called to handle each client
 I should make local variables for each thread avoid getting the wrong value
 from the global variables
***/
void *handleClient(void *new_fd) {
    //// get the argument from the thread
    int client_fd = *(int *) new_fd;

    timeOutCalc *timeOut = (timeOutCalc *) malloc(sizeof(timeOutCalc)) ;
    timeOut->start = (clock_t) clock();
    timeOut->loc = 0;
    timeOut->maxTime = 10;
    timeOut->new_fd = client_fd;

    pthread_t timeOutThread;
    pthread_create(&timeOutThread, NULL, calculateTimeOut, timeOut);
    int valread;
    char buffer[1024] = {0};

    while (timeOut->start != -1){
        timeOut->loc  = 0;         //// Make the calculateTimeOut thread start waiting the time out if the client doesn't send the request in 10 seconds
        memset(buffer, 0, sizeof buffer);         //// wait reading from the client
        valread = read(client_fd, buffer, 1024);
        if (valread == 0) {
            printf("client %d closed the connection\n", client_fd);
            break;
        } else if (valread == -1) {
            printf("error reading from the client %d\n", client_fd);
            break;
        }
        timeOut->loc  = 1;            //// Make the calculateTimeOut thread stop waiting the time out if the client sends the request in 10 seconds
        RequestType requestType = parseRequest(buffer);
        if (requestType == GET) {
            printf("GET request\n");
            handleGetRequest(client_fd, buffer);
        } else if (requestType == POST) {
            printf("POST request\n");
            handlePostRequest(client_fd, buffer);
        } else {
            printf("Invalid request\n");
            sendResponse(client_fd, not_found);
        }
        //// reset the start time for the calculateTimeOut thread then it will start waiting for 10 seconds again
        timeOut->start = (clock_t) clock();
    }

    return NULL;
}