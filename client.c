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
#define PORT 8080 // the port client will be connecting to

#define MAXDATASIZE 1024 // max number of bytes we can get at once

int sock_fd = 0, valread, client_fd;
struct sockaddr_in serv_addr;
char *hello = "Hello from client";
char buffer[MAXDATASIZE] = {0};

struct Command
{
    char *type;
    char *filePath;
    char *hostName;
    char *port;
} command;

void parse_single_command()
{
    char *token = strtok(buffer, " ");
    command.type = token;

    token = strtok(NULL, " ");
    command.filePath = token;

    token = strtok(NULL, " ");
    command.hostName = token;

    token = strtok(NULL, " ");
    command.filePath = token;
}

void parse_commands()
{
    FILE *fptr = fopen("client_commands.txt", "r");
    if (fptr == NULL)
    {
        printf("no such file.");
        return;
    }

    while (fgets(buffer, MAXDATASIZE, fptr) != NULL)
    {
        parse_single_command();

    }
}

int main(int argc, char const *argv[])
{
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((client_fd = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    send(sock_fd, hello, strlen(hello), 0);
    printf("Hello message sent\n");
    valread = read(sock_fd, buffer, 1024);
    printf("%s\n", buffer);

    close(client_fd);
    close(sock_fd);
    return 0;
}
