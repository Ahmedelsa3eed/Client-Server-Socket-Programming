cmake_minimum_required(VERSION 3.23)
project(Client_Server_Socket_Programming C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Client_Server_Socket_Programming
        client.c
        server.c)
