// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/7/20
// Description: Server epoll IO复用
#include "wrap.h"
#include "utils.h"
#include <climits>
#include <unordered_map>

using namespace std;
using namespace C_std;
using namespace C_std::Network;

Log log("./server.log");
unordered_map<int, ClientSocket> clientAddr;

int set_nonblocking(int sockfd) {
    int old_option = fcntl(sockfd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(sockfd, F_SETFL, new_option);
    return old_option;
}

void epoll_add_fd(int epfd, int sockfd, bool is_ET) {
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN;
    if (is_ET) {
        event.events |= EPOLLET;
        set_nonblocking(sockfd);
    }
    Epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
}

void epoll_del_fd(int epfd, int  sockfd) {
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN;
    Epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &event);
}

void lt(int epfd, epoll_event *event, int n, int lfd) {
    char buf[BUFSIZ];
    for (int i = 0; i < n; ++i) {
        if (event[i].events & EPOLLIN) {
            int fd = event[i].data.fd;
            if (fd == lfd) {
                //客户端连接
                ClientSocket clientSocket = easy_accept(lfd);
                clientAddr.insert({clientSocket.sockfd, clientSocket});
                log.print("[client connected] " + clientSocket.ip + ":" + to_string(clientSocket.port));
                epoll_add_fd(epfd, clientSocket.sockfd, false);
            } else {
                //客户端数据
                ClientSocket clientSocket = clientAddr[fd];
                int ret = Read(fd, buf, BUFSIZ - 1);
                if (ret > 0) {
                    buf[ret] = '\0';
                    log.print("[client " + clientSocket.ip + ":" + to_string(clientSocket.port) + "]\n{" + buf +"}");
                    Write(fd, buf, ret);
                } else {
                    epoll_del_fd(epfd, fd);
                    log.print("[client disconnect] " + clientSocket.ip + ":" + to_string(clientSocket.port));
                    clientAddr.erase(fd);
                }
            }
        }
    }
}

void et(int epfd, epoll_event *events, int n, int lfd) {
    char buf[BUFSIZ];
    for (int i = 0; i < n; ++i) {
        if (events[i].events & EPOLLIN) {
            int fd = events[i].data.fd;
            if (fd == lfd) {
                //客户端连接
                ClientSocket clientSocket = easy_accept(lfd);
                clientAddr.insert({clientSocket.sockfd, clientSocket});
                log.print("[client connected] " + clientSocket.ip + ":" + to_string(clientSocket.port));
                epoll_add_fd(epfd, clientSocket.sockfd, true);
            } else {
                //客户端发送数据
                ClientSocket clientSocket = clientAddr[fd];
                while (true) {
                    int ret = recv(fd, buf, BUFSIZ - 1, 0);
                    if (ret > 0) {
                        buf[ret] = '\0';
                        log.print("[client " + clientSocket.ip + ":" + to_string(clientSocket.port) + "]\n{" + buf +"}");
                        Write(fd, buf, ret);
                    } else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        //读数据结束
                        log.print("[client " + clientSocket.ip + ":" + to_string(clientSocket.port) + "]\n{read end}");
                        break;
                    } else {
                        //连接断开
                        epoll_del_fd(epfd, fd);
                        log.print("[client disconnect] " + clientSocket.ip + ":" + to_string(clientSocket.port));
                        clientAddr.erase(fd);
                        break;
                    }
                }
            }
        }
    }
}

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

    int epfd = Epoll_create(10);
    epoll_add_fd(epfd, lfd, true);
    constexpr int MAX_EVENT_NUMBER = 1024;
    epoll_event events[MAX_EVENT_NUMBER];

    while (true) {
        int ret = Epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
//        lt(epfd, events, ret, lfd);
        et(epfd, events, ret, lfd);
    }
    return 0;
}