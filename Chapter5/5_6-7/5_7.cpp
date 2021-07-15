// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/7/13
// Description: Server
#include "utils.h"
#include "wrap.h"
#include <cstring>

using namespace std;
using namespace C_std::Network;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s ip port\n", basename(argv[0]));
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);

    int lfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);
    Bind(lfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));

    Listen(lfd, 5);

    socklen_t socklen = sizeof(addr);
    int cfd = Accept(lfd, reinterpret_cast<sockaddr *>(&addr), &socklen);

    char buf[BUFSIZ];       //最后一个字符永远都是字符串结束符
    memset(buf, 0, BUFSIZ);
    recv(cfd, buf, BUFSIZ - 1, 0);
    printf("%s\n", buf);

    memset(buf, 0, BUFSIZ);
    recv(cfd, buf, BUFSIZ - 1, MSG_OOB);
    printf("%s\n", buf);

    memset(buf, 0, BUFSIZ);
    recv(cfd, buf, BUFSIZ - 1, 0);
    printf("%s\n", buf);

    shutdown(cfd, SHUT_RDWR);
    shutdown(lfd, SHUT_RDWR);

    return 0;
}
