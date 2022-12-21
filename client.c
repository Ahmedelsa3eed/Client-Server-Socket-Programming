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
#define MAXDATASIZE 1024 // max number of bytes we can get at once

int sock_fd = 0, client_fd;
struct sockaddr_in serv_addr;
char *http_version = " HTTP/1.1";
char commands_buff[MAXDATASIZE] = {0};
char buffer[MAXDATASIZE] = {0};

struct Command {
    char *type;
    char *filePath;
    char *hostName;
    char *port;
} command;

void parse_commands();
void parse_single_command();
void command_handler();
char* construct_get_req_header();

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

    printf("%s\n", command.hostName);
    printf("%s\n", command.port);

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
    parse_commands();

    free(command.type);
    free(command.filePath);
    free(command.hostName);
    free(command.port);
    
    close(client_fd);
    close(sock_fd);
    return 0;
}

void parse_commands()
{
    FILE *fptr = fopen("client_commands.txt", "r");
    if (fptr == NULL) {
        printf("client_commands file missing");
        return;
    }

    while (fgets(commands_buff, MAXDATASIZE, fptr) != NULL) {
        parse_single_command();
        memset(commands_buff, 0, sizeof commands_buff);
        command_handler();
    }
}

void parse_single_command()
{
    char *token = strtok(commands_buff, " ");
    command.type = (char *) malloc(strlen(token) + 1);
    strcpy(command.type, token);

    token = strtok(NULL, " ");
    command.filePath = (char *) malloc(strlen(token) + 1);
    strcpy(command.filePath, token);
    command.filePath[strlen(command.filePath) - 1] = '\0'; // remove `\n`
}



void command_handler()
{
    if (strcmp(command.type, "client_get") == 0) {
        char *req_header = construct_get_req_header();
        send(sock_fd, req_header, strlen(req_header), 0);
        printf("GET request sent to server whose addr %s\n", command.hostName);
        
        read(sock_fd, buffer, 1024);
        printf("read from server \n%s\n", buffer);
        memset(buffer, 0, sizeof buffer);
    }
    else if (strcmp(command.type, "client_post") == 0) {
        send(sock_fd, "POST req", 8, 0);
        printf("POST request sent to server whose addr %s\n", command.hostName);
    }
    else printf("Not valid command!\n");
}

char* construct_get_req_header()
{
    char *req_header = (char *) malloc(100);
    strcpy(req_header,  "GET \\");
    strcat(req_header, command.filePath);
    // strcat(req_header, http_version);
    return req_header;
}