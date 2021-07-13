#include "utils.h"
#include "wrap.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>

using namespace std;
using namespace C_std::Network;

namespace {
    int listen_fd;
}

static void accept_connect(int signo) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd;
    client_fd = Accept(listen_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);
    char ip[INET_ADDRSTRLEN];
    printf("client: %s %d\n", inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN),
           ntohs(client_addr.sin_port));
    shutdown(client_fd, SHUT_RDWR);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s ip port\n", basename(argv[0]));
        exit(1);
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);
    Bind(listen_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));

    Listen(listen_fd, 5);

    signal(SIGQUIT, accept_connect);

    pause();

    shutdown(listen_fd, SHUT_RDWR);

    return 0;
}