#pragma once

#include <unistd.h>

#include "network.h"

class TcpSocket: public Socket {
    friend class TcpNetwork;

    int fd_;

    // make constructor private, so it could only be created by friend class TcpNetwork
    TcpSocket();
    TcpSocket(int fd): fd_(fd) {}
public:
    ~TcpSocket();

    int bind(const char*);
    Socket* accept();
    int close();
    int set(Socket::Option, ...);

    int read(void* data, int n) {
        return ::read(fd_, data, n);
    }

    int write(const void* data, int n) {
        return ::write(fd_, data, n);
    }

    int fd() {
        return fd_;
    }

    int connect(const char* addr);
};

class TcpNetwork: public Network {
public:
    ~TcpNetwork() {}

    TcpSocket* makeSocket();
};

