#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libgen.h>
#define MAXDATASIZE 1024 // 4k bytes: max number of bytes we can get at once

int sock_fd = 0, client_fd;
struct sockaddr_in serv_addr;
char *httpVersion = " HTTP/1.1";
char *postStartLine = "POST / HTTP/1.1\r\n";
char commands_buff[MAXDATASIZE] = {0};
char buffer[MAXDATASIZE] = {0};

struct Command {
    char *type;
    char *filePath;
    char *hostName;
    char *port;
} command;

typedef enum ResponseType {
    OK,
    ERROR
} ResponseType;

void parseCommands();
void parseSingleCommand();
void commandHandler();
char* constructGetReqHeader();
void readFile(char *fileName);
void saveFile(char *filePath, size_t nBytes, size_t startLineSize);
enum ResponseType parseResponse(char *res);
char *getResStartLine();

int main(int argc, char const *argv[])
{
    if (argc > 1) {
        command.hostName = (char *)malloc(strlen(argv[1]) + 1);
        strcpy(command.hostName, argv[1]);
    }
    if (argc > 2) {
        command.port = (char *)malloc(strlen(argv[2]) + 1);
        strcpy(command.port, argv[2]);
    }
    else {
        command.port = "8080";
    }

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(command.port));
    // Convert IPv4 addresses from text to binary form
    if (inet_pton(AF_INET, command.hostName, &serv_addr.sin_addr) == -1) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((client_fd = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) == -1) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("client: connected... \n");
    parseCommands();

    free(command.type);
    free(command.filePath);
    free(command.hostName);
    free(command.port);
    
    close(client_fd);
    close(sock_fd);
    return 0;
}

void parseCommands()
{
    FILE *fptr = fopen("client_commands.txt", "r");
    if (fptr == NULL) {
        printf("client_commands file missing");
        return;
    }

    while (fgets(commands_buff, MAXDATASIZE, fptr) != NULL) {
        parseSingleCommand();
        memset(commands_buff, 0, sizeof commands_buff);
        commandHandler();
    }
}

void parseSingleCommand()
{
    char *token = strtok(commands_buff, " ");
    command.type = (char *) malloc(strlen(token) + 1);
    strcpy(command.type, token);

    token = strtok(NULL, " ");
    command.filePath = (char *) malloc(strlen(token) + 1);
    strcpy(command.filePath, token);
}

void commandHandler()
{
    if (strcmp(command.type, "client_get") == 0) {
        char *req_header = constructGetReqHeader();
        send(sock_fd, req_header, strlen(req_header), 0);
        free(req_header);
        size_t nBytes = read(sock_fd, buffer, MAXDATASIZE);
        char *resStartLine = getResStartLine();
        saveFile(command.filePath, nBytes, strlen(resStartLine));
        memset(buffer, 0, sizeof(buffer));
    }
    else if (strcmp(command.type, "client_post") == 0) {
        send(sock_fd, postStartLine, strlen(postStartLine), 0);
        printf("POST request sent to server");
        char resBuffer[1024] = {0};
        read(client_fd, resBuffer, 1024);
        if (parseResponse(resBuffer) == OK) {
            readFile(command.filePath);
            send(sock_fd, buffer, MAXDATASIZE, 0);
            memset(buffer, 0, sizeof(buffer));
        }
    }
    else printf("Not valid command!\n");
}

char* constructGetReqHeader()
{
    char *req_header = (char *) malloc(100);
    strcpy(req_header,  "GET /");
    strcat(req_header, command.filePath);
    // strcat(req_header, httpVersion);
    return req_header;
}

char *getResStartLine()
{
    char *header = (char *) malloc(50);
    header = strtok(buffer, "\r\n");
    printf("%s\n", header);
    return header;
}

void readFile(char *fileName)
{
    FILE *fptr;
    if ((fptr = fopen(fileName, "rb")) == NULL) {
        fprintf(stderr, "Couldn't open file for reading. \n");
        return;
    }
    unsigned int nBytes = 0;
    while(fread(&buffer[nBytes], sizeof(char), 1, fptr) == 1) {
        nBytes++;
    }
    fclose(fptr);
}

void saveFile(char *filePath, size_t nBytes, size_t startLineSize)
{
    char *newfilePath = (char *) malloc(50);
    strcat(newfilePath, "clientGetFiles/");
    strcat(newfilePath, basename(filePath));

    FILE *fptr;
    if ((fptr = fopen(newfilePath, "wb")) == NULL) {
        fprintf(stderr, "Couldn't open file for writing. \n");
        return;
    }
    int toBeWritten = startLineSize + 2;
    while (toBeWritten < nBytes) {
        fwrite(&buffer[toBeWritten], sizeof(char), 1, fptr);
        toBeWritten++;
    }
    free(newfilePath);
    fclose(fptr);
}

enum ResponseType parseResponse(char *res)
{
    if (strstr(res, "OK") != NULL)
        return OK;
    else
        return ERROR;
}