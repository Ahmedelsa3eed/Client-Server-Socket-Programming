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
#define PORT 8080 // the port users will be connecting to

#define BACKLOG 10 // how many pending connections queue will hold

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

int main(int argc, char const *argv[])
{
    // listen on server_fd, new connection on new_fd
    int server_fd, new_fd, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    struct sigaction sa;
    char buffer[1024] = {0};
    char *ok = "HTTP/1.1 200 OK\r\n";
    char *not_found = "HTTP/1.1 404 Not Found\r\n";

    // use AF_LOCAL for communication between processes on the same host
    // For communicating between processes on different hosts connected by IPV4, we use AF_INET
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < -1) {
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
        // parsing command and specify if it GET or POST
        // if target file does not exists return error
        // transmit content of file (reads from the file and write on the socket) - in case of GET
        // wait for new requests (presistent connection)
        // close if connection timed out

        printf("server: got connection\n");
        if (!fork()) { // this is the child process
            close(server_fd); // child doesn't need the listener
            valread = read(new_fd, buffer, 1024);
            printf("%s\n", buffer);
            if ((send(new_fd, ok, strlen(ok), 0)) < 0) {
                perror("send failed");
                exit(EXIT_FAILURE);
            }
            printf("Hello message sent\n");
            close(new_fd);
            exit(EXIT_SUCCESS);
        }
        close(new_fd);  // parent doesn't need this
    }

    if ((shutdown(server_fd, SHUT_RDWR)) < 0) {
        perror("shutdown failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}