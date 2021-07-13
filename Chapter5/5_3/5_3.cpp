#include "wrap.h"
#include <csignal>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>

using namespace std;
using namespace C_std::Network;

static bool stop = false;
static void handle_term (int signo) {
    stop = true;
}

int main(int argc, char *argv[]) {
    signal(SIGTERM, handle_term);
    if (argc != 4) {
        printf("usage: %s ip port backlog\n", basename(argv[0]));
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

//    printf("ip = %s\nport = %d\nbacklog = %d\n", ip, port, backlog);

    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    Bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

    Listen(sockfd, backlog);

    while (!stop) {
        sleep(1);
    }

    close(sockfd);
    return 0;
}