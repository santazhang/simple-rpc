#include <string>
#include <stdarg.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "utils.h"
#include "tcp_network.h"

using namespace std;

TcpSocket::TcpSocket() {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
}

TcpSocket::~TcpSocket() {
    close();
}

static void parse_sock_addr(const char* addr, sockaddr_in* bind_addr) {
    string s = addr;
    int idx = s.find(":");
    string host_str = s.substr(0, idx);
    const char* host = host_str.c_str();
    string port_str = s.substr(idx + 1);
    int port = atoi(port_str.c_str());

    bzero(bind_addr, sizeof(*bind_addr));
    bind_addr->sin_family = AF_INET;
    bind_addr->sin_addr.s_addr = inet_addr(host);
    bind_addr->sin_port = htons(port);
}

int TcpSocket::bind(const char* addr) {
    sockaddr_in bind_addr;
    parse_sock_addr(addr, &bind_addr);

    const int yes = 1;
    VERIFY(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == 0);

    VERIFY(::bind(fd_, (sockaddr*) &bind_addr, sizeof(bind_addr)) == 0);
    const int backlog = 100;
    VERIFY(::listen(fd_, backlog) == 0);
    return 0;
}

Socket* TcpSocket::accept() {
    sockaddr_in clnt_addr;
    socklen_t sin_sz = sizeof(clnt_addr);
    int fd = ::accept(this->fd_, (sockaddr*) &clnt_addr, &sin_sz);
    if (fd < 0) {
        return NULL;
    } else {
        return new TcpSocket(fd);
    }
}

int TcpSocket::close() {
    Log::debug("closing socket with fd=%d", fd_);
    return ::close(fd_);
}

int TcpSocket::set(Socket::Option opt, ...) {
    int ret = 0;
    va_list va;
    va_start(va, opt);
    switch(opt) {
    case NONBLOCKING:
        bool nonblocking = va_arg(va, int); // bool will be promoted to int
        ret = setnonblocking(fd_, nonblocking);
        break;
    }
    va_end(va);
    return ret;
}

int TcpSocket::connect(const char* addr) {
    sockaddr_in bind_addr;
    parse_sock_addr(addr, &bind_addr);

    int ret = ::connect(fd_, (sockaddr*) &bind_addr, sizeof(bind_addr));
    VERIFY(ret == 0);
    return ret;
}

TcpSocket* TcpNetwork::makeSocket() {
    return new TcpSocket;
}

