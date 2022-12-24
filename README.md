# Client Server with Socket Programming in C/C++

## Introduction
Using sockets to implement a simple web client that communicates with a web server using a restricted subset of HTTP.

## Technologies
- Socket Programming in C
- Linux platform

## Code organization

### Client

#### Data Structures
- ``struct Command``: Structure that holds the command type (GET or POST), filePath, hostName, and port number.
- ``enum ResponseType``: Enum that takes possible values ``OK`` or ``ERROR``.
- ``struct sockaddr_in``: Structure describing an Internet socket address.
- ``char commands_buff[]``: Char array that holds the commands read from the client input file.
- ``char buffer[]``: Char array that holds the req or res payload.

#### Major Functions
- ``parseCommands()``: it parses the commands read from the client input file.
- ``commandHandler()``: it process the parsed command and determines if it is ``GET`` or ``POST``

### Server

#### Data Structures
- ``enum RequestType``: Enum that takes possible values ``GET``, ``POST`` or ``WrongReq``.
- ``struct timeOutCalc``: Structure that holds the thread start time, loc variable, max time, client file discriptor.
- ``int clientSockets[MaxClients]``: Integer array that holds the fd of each client.

#### Major Functions
- ``RequestType parseRequest()``: it parses the client req and returns the req type.
- ``handleGetRequest()``: it process the client ``GET`` req.
- ``handlePostRequest()``: it process the client ``POST`` req.
- ``calculateTimeOut()``: it calculates the timeout value of the client based on some hurestic.

