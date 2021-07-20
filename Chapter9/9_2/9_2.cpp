// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/7/20
// Description: Server : poll实现IO复用

#include "wrap.h"
#include "utils.h"
#include <unordered_map>

using namespace std;
using namespace C_std;
using namespace C_std::Network;

constexpr int MAX_EVENT_NUMBER = 1024;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("[usage] %s ip port\n", basename(argv[0]));
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);

    int lfd = Socket(AF_INET, SOCK_STREAM, 0);
    port_reuse(lfd);
    easy_bind(lfd, port, ip);
    Listen(lfd, 128);

    pollfd fds[MAX_EVENT_NUMBER];
    for (int i = 0; i < MAX_EVENT_NUMBER; ++i) {
        fds[i].fd = -1;
    }
    int len = 1;    //空递增

    fds[0].fd = lfd;
    fds[0].events = POLLIN;

    Log log("./server.log");
    char buf[BUFSIZ];
    unordered_map<int, ClientSocket> clientAddr;

    while (true) {
        int ret = Poll(fds, len, -1);
        if (fds[0].revents & POLLIN) {
            //客户端连接事件
            ClientSocket clientSocket = easy_accept(lfd);
            log.print("[client connected] " + clientSocket.ip + ":" + to_string(clientSocket.port));
            int i;
            for (i = 0; i < MAX_EVENT_NUMBER; ++i) {
                if (fds[i].fd == -1) {
                    fds[i].fd = clientSocket.sockfd;
                    fds[i].events = POLLIN;
                    len = max(i + 1, len);
                    clientAddr.insert({clientSocket.sockfd, clientSocket});
                    break;
                }
            }
            if (i == MAX_EVENT_NUMBER) {
                perror_exit("TOO MANY EVENTS");
            }
            if (--ret == 0) continue;
        }

        for (int i = 0; i < len; ++i) {
            if (fds[i].fd == -1) continue;
            if (fds[i].revents & POLLIN) {
                int fd = fds[i].fd;
                auto &clientSocket = clientAddr[fd];
                int n = Read(fd, buf, BUFSIZ - 1);
                if (n > 0) {
                    //读取客户端发送数据
                    buf[n] = '\0';  //手动添加字符串结束符
                    log.print("[client " + clientSocket.ip + ":" + to_string(clientSocket.port) + "]\n{" + buf +"}");
                    Write(fd, buf, n);
                } else {
                    //客户端断开或出错
                    shutdown(fd, SHUT_RDWR);
                    log.print("[client disconnect] " + clientSocket.ip + ":" + to_string(clientSocket.port));
                    clientAddr.erase(fd);
                    fds[i].fd = -1;
                    if (i + 1 == len) {
                        //如果最大的文件描述符断开，则需要更新len的长度
                        for (int j = i - 1; j >= 0; --j) {
                            if (fds[j].fd != -1) {
                                len = j + 1;
                                break;
                            }
                        }
                    }
                }
                if (--ret == 0) break;
            }
        }
    }
    return 0;
}