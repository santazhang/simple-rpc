#pragma once

struct Socket {
    typedef enum {
        NONBLOCKING = 0x9001
    } Option;

    virtual ~Socket() = 0;

    virtual int bind(const char* addr) = 0;
    virtual Socket* accept() = 0;
    virtual int close() = 0;
    virtual int set(Option, ...) = 0;
    virtual int read(void* data, int n) = 0;
    virtual int write(const void* data, int n) = 0;
    virtual int fd() = 0;
    virtual int connect(const char* addr) = 0;
};
inline Socket::~Socket() {}

struct Network {
    virtual ~Network() = 0;
    virtual Socket* makeSocket() = 0;
};
inline Network::~Network() {}
