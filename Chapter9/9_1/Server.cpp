// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/7/19
// Description: Server select实现IO复用

#include "wrap.h"
#include "utils.h"
#include <cstring>
#include <unordered_set>
#include <unordered_map>

using namespace std;
using namespace C_std::Network;
using namespace C_std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s ip port\n", basename(argv[0]));
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);

    int lfd = Socket(AF_INET, SOCK_STREAM, 0);
    //设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    easy_bind(lfd, port, ip);
    Listen(lfd, 5);



    unordered_set<int> fdset;       //用来保存连接的客户端socket
    unordered_map<int, ClientSocket> clientAddr;
    int nfds;                       //监听的最大fd + 1
    Log log("./server.log");

    fd_set allfds, readfds;
    FD_ZERO(&allfds);
    //首先将lfd加入select监听
    FD_SET(lfd, &allfds);
    nfds = lfd + 1;
    char buf[BUFSIZ];

    while (true) {
        readfds = allfds;
        int n = Select(nfds, &readfds, nullptr, nullptr, nullptr);

        if (FD_ISSET(lfd, &readfds)) {
            //客户端发起连接
            ClientSocket clientSocket = easy_accept(lfd);
            log.print("[client connected] " + clientSocket.ip + ":" + to_string(clientSocket.port));
            int cfd = clientSocket.sockfd;
            nfds = max(nfds, cfd + 1);
            clientAddr.insert({cfd, clientSocket});
            fdset.insert(cfd);
            FD_SET(cfd, &allfds);

            if (--n == 0) continue;       //没有其他文件描述符就绪
        }


        for (auto cfd = fdset.begin(); cfd != fdset.end(); ) {
            if (FD_ISSET(*cfd, &readfds)) {
                int ret = Read(*cfd, buf, BUFSIZ - 1);
                auto clientSocket = clientAddr[*cfd];
                if (ret > 0) {
                    buf[ret] = '\0';
                    log.print("[client " + clientSocket.ip + ":" + to_string(clientSocket.port) + "]\n{" + buf +"}");
                    Write(*cfd, buf, ret);
                    ++cfd;
                } else {
                    shutdown(*cfd, SHUT_RDWR);
                    FD_CLR(*cfd, &allfds);
                    log.print("[client disconnect] " + clientSocket.ip + ":" + to_string(clientSocket.port));
                    if (*cfd + 1 == nfds) {
                        for (auto cfd : fdset) {
                            nfds = max(nfds, cfd + 1);
                        }
                    }

                    //删除cfd
                    clientAddr.erase(*cfd);
                    fdset.erase(cfd++);
                }
                if (--n == 0) continue;       //没有其他文件描述符就绪
            } else {
                ++cfd;
            }
        }
    }

    shutdown(lfd, SHUT_RDWR);
    return 0;
}