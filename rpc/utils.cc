#include <utility>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "utils.h"

using namespace std;

namespace rpc {

int set_nonblocking(int fd, bool nonblocking) {
    int ret = fcntl(fd, F_GETFL, 0);
    if (ret != -1) {
        if (nonblocking) {
            ret = fcntl(fd, F_SETFL, ret | O_NONBLOCK);
        } else {
            ret = fcntl(fd, F_SETFL, ret & ~O_NONBLOCK);
        }
    }
    return ret;
}

int open_socket(const char* addr, const struct addrinfo* hints,
                std::function<bool(int, const struct sockaddr*, socklen_t)> filter /* =? */,
                struct sockaddr** p_addr /* =? */, socklen_t* p_len /* =? */) {

    int sock = -1;
    string str_addr(addr);
    size_t idx = str_addr.find(":");
    if (idx == string::npos) {
        Log_error("open_socket(): bad address: %s", addr);
        return -1;
    }
    string host = str_addr.substr(0, idx);
    string port = str_addr.substr(idx + 1);

    struct addrinfo *result, *rp;
    int r = getaddrinfo((host == "0.0.0.0") ? nullptr : host.c_str(), port.c_str(), hints, &result);
    if (r != 0) {
        Log_error("getaddrinfo(): %s", gai_strerror(r));
        return -1;
    }

    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        } else if (filter != nullptr && filter(sock, rp->ai_addr, rp->ai_addrlen) == false) {
            close(sock);
            sock = -1;
            continue;
        } else {
            break;
        }
    }

    if (rp == nullptr) {
        Log_error("open_socket(): failed to open proper socket %s", strerror(errno));
        sock = -1;
    } else if (p_addr != nullptr && p_len != nullptr) {
        *p_addr = (struct sockaddr *) malloc(rp->ai_addrlen);
        *p_len = rp->ai_addrlen;
        memcpy(*p_addr, rp->ai_addr, *p_len);
    }

    freeaddrinfo(result);
    return sock;
}

}
