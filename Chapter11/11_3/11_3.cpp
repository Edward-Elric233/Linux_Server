// Copyright(C), Edward-Elric233
// Author: Edward-Elric233
// Version: 1.0
// Date: 2021/7/29
// Description: Server with IO复用、统一事件源、计时器，不使用wraper封装，回忆API

#include "11_3.h"
#include "wrap.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <cassert>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sys/time.h>

using namespace std;
using namespace C_std;
using namespace C_std::Network;
using namespace data_structure;

namespace {
    int lfd, efd;
    int pp[2];      //pp[0]读，pp[1]写
    unordered_map<int, function<void(void)>> sig_handler = {
            {SIGHUP, []() {
                cout << "[SIGNAL] SIGHUP has been catched" << endl;
            }},
            {SIGINT, []() {
                cout << "[SIGNAL] SIGINT has been catched" << endl;
            }},
            {SIGQUIT, []() {
                cout << "[SIGNAL] SIGQUIT has been catched" << endl;
            }},
            {SIGTERM, []() {
                cout << "[SIGNAL] SIGTERM has been catched\n[TERM]" << endl;
                exit(1);
            }},
            {SIGALRM, []() {
                cout << "[SIGNAL] SIGALRM has been catched" << endl;
                time_t cur = time(nullptr);
                for (auto timer = timerSet.begin(); timer != timerSet.end(); ) {
                    if (timer->expire < cur){
                        timer->cb((timer++)->clientData->sockfd);
                    } else {
                        timer++;
                    }
                }
            }},
    };
    constexpr int MAX_EVENTS = 1024;
    struct epoll_event events[MAX_EVENTS];
    char buf[BUFSIZ];
    constexpr time_t INTERVAL = 10;     //服务器每隔10s检查一次半分钟内未发送数据的客户端并断开连接
}

int set_nonblocking(int sockfd) {
    int old_fl = fcntl(sockfd, F_GETFL);
    int new_fl = old_fl | O_NONBLOCK;
    fcntl(sockfd, F_SETFL, new_fl);
    return old_fl;
}

void epoll_addfd(int sockfd) {
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET;
    assert(epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &event) == 0);
    set_nonblocking(sockfd);
}

void epoll_delfd(int sockfd) {
    auto &clientData = clientMap.at(sockfd);
    cout << "[" << clientData.ip << ":" << clientData.port << "] " << "disconnected" << endl;
    epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, nullptr);
    shutdown(sockfd, SHUT_RDWR);
    timerSet.erase(clientData.timer);
    clientMap.erase(sockfd);
}

void signal_handle(int signo) {
    assert(write(pp[1], &signo, 1) == 1);
}

void signal_catch() {
    for (auto &iter : sig_handler) {
        int signo = iter.first;
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        //原本以为必须是restart，否则epoll_wait函数被中断以后不会继续监听，但是经过测试后发现可能epoll_wait不属于被重新启动的系统调用
        //经过思考，使用restart信号便不会影响read等函数，因此还是很有必要的
        //本来有点担心，如果在read的过程中被信号打断会不会出现吞掉数据的情况：对方以为服务器已经读了，但是实际上失败了
        //但是经过测试并没有产生这种现象，可能是因为tcp是面向连接的可靠传输吧，read的在实现的时候如果出现错误应该不会返回读取成功的ack，这样对方还会再传输一次的
        //感觉tcp好难实现啊
        act.sa_flags |= SA_RESTART;
        //act.sa_flags = 0;
        sigfillset(&act.sa_mask);   //将其他所有信号屏蔽
        act.sa_handler = signal_handle;
        assert(sigaction(signo, &act, nullptr) == 0);
    }
}

int main(int argc, char *argv[]) {
    ios::sync_with_stdio(false);
    if (argc != 3) {
        cout << "[usage] " << basename(argv[0]) << " ip port" << endl;
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(lfd > 0);
    //设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = ntohs(port);
    assert(inet_pton(AF_INET, ip, &client_addr.sin_addr.s_addr) > 0);
    assert(bind(lfd, reinterpret_cast<sockaddr *>(&client_addr), sizeof(client_addr)) == 0);
    assert(listen(lfd, 5) == 0);

    efd = epoll_create(5);
    assert(efd > 0);

    epoll_addfd(lfd);

    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pp) == 0);
    epoll_addfd(pp[0]);
    set_nonblocking(pp[1]);     //书上将写端设置为非阻塞，虽然我不知道为什么

    signal_catch();

    struct itimerval timerval;
    timerval.it_value = {INTERVAL, 0};
    timerval.it_interval = {INTERVAL, 0};
    assert(setitimer(ITIMER_REAL, &timerval, nullptr) == 0);

    while (true) {
        int n = epoll_wait(efd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) {
                //被信号中断
                //cout << "epoll_wait was interruped" << endl;
                continue;
            }
            perror("epoll_wait error");
            exit(1);
        }
        for (int i = 0; i < n; ++i) {
            if (events[i].events & EPOLLIN) {
                int fd = events[i].data.fd;
                if (fd == lfd) {
                    //客户端连接请求
                    int cfd;
                    socklen_t addrlen = sizeof(client_addr);
                    while ((cfd = accept(lfd, reinterpret_cast<struct sockaddr *>(&client_addr), &addrlen)) > 0) {
                        clientMap.insert({cfd, ClientData(cfd, inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, buf, INET_ADDRSTRLEN), ntohs(client_addr.sin_port))});
                        auto &clientData = clientMap.at(cfd);
                        cout << "[client connected] " << clientData.ip << " " << clientData.port << endl;
                        timerSet.insert(Timer(time(nullptr) + 3 * INTERVAL, epoll_delfd, &clientData));
                        clientData.timer = std::find_if(timerSet.begin(), timerSet.end(), [&clientData](const Timer& x){
                            return x.clientData == &clientData;
                        });
                        epoll_addfd(cfd);
                    }
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    } else {
                        perror("accept error");
                        //TODO:不应该因为某个客户端的行为退出，应该添加出错处理
                        exit(1);
                    }
                } else if (fd == pp[0]) {
                    //信号中断
                    int ret = read(pp[0], buf, BUFSIZ);
                    assert(ret > 0);
                    for (int ii = 0; ii < ret; ++ii) {
                        sig_handler[buf[ii]]();
                    }
                    continue;
                } else {
                    //客户端发送数据
                    int ret;
                    while ((ret = read(fd, buf, BUFSIZ - 1)) > 0) {
                        buf[ret] = '\0';        //手动添加字符串结束符
                        auto &clientData = clientMap.at(fd);
                        cout << "[" << clientData.ip << ":" << clientData.port << "] " << buf << endl;       //echo服务器
                        assert(write(fd, buf, ret) > 0);
                        timerSet.erase(clientData.timer);
                        timerSet.insert(Timer(time(nullptr) + 3 * INTERVAL, epoll_delfd, &clientData));
                        clientData.timer = std::find_if(timerSet.begin(), timerSet.end(), [&clientData](const Timer& x){
                            return x.clientData == &clientData;
                        });
                    }
                    if (ret == 0) {
                        //客户端断开连接
                        epoll_delfd(fd);
                    } else if (ret == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        } else {
                            perror("read error");
                            //TODO:不应该因为某个客户端的行为退出，应该添加出错处理
                            exit(1);
                        }
                    }
                    continue;
                }
            }
        }
    }

    shutdown(lfd, SHUT_RDWR);
    shutdown(efd, SHUT_RDWR);
    return 0;

}