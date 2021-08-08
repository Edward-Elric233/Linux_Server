// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/7/23
// Description: Client Connect超时设置
#include "wrap.h"
#include "utils.h"

using namespace std;
using namespace C_std::Network;
using namespace C_std;

int timeout_connect(const string &ip, int port, int timeout_sec) {
    int cfd = Socket(AF_INET, SOCK_STREAM, 0);

    //使用setsockopt给cfd设置超时事件
    struct timeval timeout;
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);

    int ret = connect(cfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (ret == -1) {
        printf("connect error\n");
        if (errno == EINPROGRESS) {
            printf("timeout\n");
        }
    }
    return cfd;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        printf("[usage]: %s ip port [timeout]\n", basename(argv[0]));
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int timeout = 10;
    if (argc == 4) {
        timeout = atoi(argv[3]);
    }

    int cfd = timeout_connect(ip, port, timeout);
    printf("connect sucess\n");

    pause();

    return 0;
}

