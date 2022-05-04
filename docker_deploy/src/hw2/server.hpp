#ifndef MY_SERVER_H
#define MY_SERVER_H
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "myexception.hpp"

using namespace std;

class Server {
    private:
        int status;
        int socket_fd;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        const char *hostname;
        const char *port;
        string client_ip;
    public:
        Server() : status(0), socket_fd(0), hostname(NULL), port(NULL) {}
        Server(const char *port_num) : status(0), socket_fd(0), hostname(NULL), port(port_num) {
            init_server();
        }
        ~Server() {
            freeaddrinfo(host_info_list);
            close(socket_fd);
        }
        void init_server();
        int accept_request();
        string get_client_ip();
};

#endif