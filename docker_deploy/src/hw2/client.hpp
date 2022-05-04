#ifndef MY_CLIENT_H
#define MY_CLIENT_H
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "myexception.hpp"

using namespace std;

class Client {
    private:
        int status;
        int socket_fd;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        const char *hostname;
        const char *port;
    public:
        Client() : status(0), socket_fd(0), hostname(NULL), port(NULL) {};
        Client(const char *port_num, const char *host_addr) : status(0), socket_fd(0), hostname(host_addr), port(port_num){
            init_client();
        };
        ~Client() {
            freeaddrinfo(host_info_list);
            close(socket_fd);
        }
        void init_client();
        int get_server_fd();
};

#endif