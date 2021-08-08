// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/7/22
// Description: 统一事件源
#include "wrap.h"
#include "utils.h"
#include <signal.h>
#include <climits>
#include <unordered_map>
#include <initializer_list>

using namespace std;
using namespace C_std;
using namespace C_std::Network;

namespace {
    Log log("./server.log");
    unordered_map<int, ClientSocket> clientAddr;
    int pipefd[2];  //管道
    int lfd, epfd;
    constexpr int MAX_EVENT_NUMBER = 1024;
    epoll_event events[MAX_EVENT_NUMBER];
}


int set_nonblocking(int sockfd) {
    int old_option = fcntl(sockfd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(sockfd, F_SETFL, new_option);
    return old_option;
}

void epoll_add_fd(int sockfd, bool is_ET) {
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN;
    if (is_ET) {
        event.events |= EPOLLET;
        set_nonblocking(sockfd);
    }
    Epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
}

void epoll_del_fd(int  sockfd) {
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN;
    Epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &event);
}

void epoll_exit() {
    close(lfd);
    close(pipefd[0]);
    close(pipefd[1]);
    log.print("[EXIT]");
    exit(1);
}

void et(int n) {
    char buf[BUFSIZ];
    for (int i = 0; i < n; ++i) {
        if (events[i].events & EPOLLIN) {
            int fd = events[i].data.fd;
            if (fd == lfd) {
                //客户端连接
                ClientSocket clientSocket = easy_accept(lfd);
                clientAddr.insert({clientSocket.sockfd, clientSocket});
                log.print("[client connected] " + clientSocket.ip + ":" + to_string(clientSocket.port));
                epoll_add_fd(clientSocket.sockfd, true);
            } else if (fd == pipefd[0]) {
                int ret = Read(fd, buf, BUFSIZ - 1);
                for (int i = 0; i < ret; ++i) {
                    switch (buf[i]) {
                        case SIGHUP:
                            log.print("[signal catched] SIGHUP");
                            break;
                        case SIGCHLD:
                            log.print("[signal catched] SIGCHLD");
                            break;
                        case SIGTERM:
                            log.print("[signal catched] SIGTERM");
                            break;
                        case SIGINT:
                            log.print("[signal catched] SIGINT");
                            epoll_exit();
                            break;
                        case SIGQUIT:
                            log.print("[signal catched] SIGQUIT");
                            break;
                    }
                }
            } else {
                //客户端发送数据
                ClientSocket clientSocket = clientAddr[fd];
                while (true) {
                    int ret = Read(fd, buf, BUFSIZ - 1);
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
                        epoll_del_fd(fd);
                        log.print("[client disconnect] " + clientSocket.ip + ":" + to_string(clientSocket.port));
                        clientAddr.erase(fd);
                        break;
                    }
                }
            }
        }
    }
}

void epoll_listen(const string &ip = "127.0.0.1", int port = 12345) {
    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    port_reuse(lfd);
    easy_bind(lfd, port, ip.c_str());
    Listen(lfd, 3);

    epfd = Epoll_create(10);
    epoll_add_fd(lfd, true);
}

void sig_handler(int signo) {
    //保留原来的errno，保证函数的可重入性
    int old_errno = errno;
    Write(pipefd[1], &signo, 1);
    errno = old_errno;
}

void epoll_signal(initializer_list<int> sigs) {
    //只能使用本地套接字
    check_error(socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd), "socketpair error");
    set_nonblocking(pipefd[1]);
    epoll_add_fd(pipefd[0], false);
    for (auto sig : sigs) {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_flags |= SA_RESTART;
        sigfillset(&act.sa_mask);
        act.sa_handler = sig_handler;
        check_error(sigaction(sig, &act, nullptr), "sigaction error");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("[usage] %s ip port\n", basename(argv[0]));
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);

    epoll_listen(ip, port);

    epoll_signal({SIGHUP, SIGCHLD, SIGTERM, SIGINT, SIGQUIT});

    while (true) {
        int ret = Epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
        et(ret);
    }
    return 0;
}
