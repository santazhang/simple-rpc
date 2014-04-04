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

int find_open_port() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    addrinfo *local_addr;

    addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo("0.0.0.0", nullptr, nullptr, &local_addr) != 0) {
        Log_error("Failed to getaddrinfo");
        return -1;
    }

    int port = -1;

    for (int i = 1024; i < 65000; ++i) {
        ((sockaddr_in*)local_addr->ai_addr)->sin_port = i;
        if (::bind(fd, local_addr->ai_addr, local_addr->ai_addrlen) != 0) {
            continue;
        }

        sockaddr_in addr;
        socklen_t addrlen;
        memset(&addr, 0, sizeof(addr));
        if (getsockname(fd, (sockaddr*)&addr, &addrlen) != 0) {
            Log_error("Failed to get socket address");
            return -1;
        }

        port = i;
        break;
    }

    freeaddrinfo(local_addr);
    ::close(fd);

    if (port != -1) {
        Log_info("Found open port: %d", port);
        return port;
    }

    Log_error("Failed to find open port.");
    return -1;
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

std::string get_host_name() {
#ifdef __APPLE__
    const int hostname_max = _POSIX_HOST_NAME_MAX + 1;
#else
    const int hostname_max = HOST_NAME_MAX + 1;
#endif // __APPLE__
    char buffer[hostname_max];
    if (gethostname(buffer, hostname_max) != 0) {
        Log_error("Failed to get hostname.");
        return "";
    }
    return std::string(buffer);
}

}
