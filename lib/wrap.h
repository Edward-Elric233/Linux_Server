//
// Created by edward on 2021/6/3.
//

#ifndef LINUX_NET_WRAP_H
#define LINUX_NET_WRAP_H

#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#include "utils.h"

namespace C_std {

    /*!
     * raed函数：从文件描述符fd中读取字符，添加出错处理
     * @param fd
     * @param buf
     * @param count
     * @return
     */
    ssize_t Read(int fd, void *buf, size_t count);

    /*!
     * write函数：向文件描述符fd中写入字符，添加出错处理
     * @param fd
     * @param buf
     * @param count
     * @return
     */
    ssize_t Write(int fd, const void *buf, size_t count);

    /*!
     * open函数
     * @param pathname
     * @param flags
     * @param mode
     * @return
     */
    int Open(const char *pathname, int flags, mode_t mode);

    /*!
     * close函数
     * @param fd        需要关闭的文件描述符
     * @return          成功0，失败-1
     */
    int Close(int fd);

    namespace IPC {
        /*!
         * mmap函数：创建内存映射文件，用于IPC
         * @param addr          建立映射区的首地址，由Linux内核制定，使用时直接传null
         * @param length        创建映射区的大小
         * @param port          映射区权限：PROT_READ PROT_WRITE
         * @param flags         标志位参数 MAP_SHARED MAP_PRIVATE MAP_ANON
         * @param fd            用来建立映射区的文件描述符，如果是匿名映射区则为-1
         * @param offset        映射文件的偏移量（4k的整数倍）
         * @return              成功返回映射区的首地址，失败返回MAP_FAILED
         */
        void *Mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

        /*!
         * munmap函数：释放内存映射文件
         * @param addr          mmap返回值
         * @param length        内存映射文件的大小
         * @return              成功0，失败-1
         */
        int Munmap(void *addr, size_t length);
    }



    namespace Network{

        /*!
         * socket函数：创建socket，并进行出错处理
         * @param domain    协议类型，常用的为AF_INET，表示IPv4，AF_INET6表示IPv6
         * @param type      通信语义，常用的为SOCK_STREAM
         * @param protocol  通信协议，常用为0，表示该通信语义下的默认协议，SOCK_STREAM的默认协议为TCP
         * @return          返回所创建socket的文件描述符，失败返回-1
         */
        int Socket(int domain, int type, int protocol);

        /*!
         * 设置端口复用
         * @param sockfd
         */
        void port_reuse(int sockfd);

        /*!
         * bind函数：将socket与对应的IP地址和端口绑定，如果不手动绑定在accept或connect的是时候会自动对socket进行绑定到本地，端口随机
         * @param sockfd    socket文件描述符
         * @param addr      sockaddr类型，描述通信类型、IP地址、端口
         * @param addrlen   传入的addr的大小，因为实际传入的addr经常是其他类型（例如sockaddr_in）转换而来，通过addrlen来判断是哪种类型
         * @return          成功返回0，失败返回-1
         */
        int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

        /*!
         * 加强版bind函数：原本的bind函数虽然功能强大，但是在简单的编程中往往不需要设置那么复杂的参数，通过easy_bind一键绑定
         * @param sockfd    socket文件描述符
         * @param port      端口
         * @param ip        IP地址，默认是本地地址
         * @return          成功返回0，失败返回-1
         */
        int easy_bind(int sockfd, int port, const std::string &ip = "127.0.0.1");

        /*!
         * listen函数：创建监听队列以存放待处理的客户连接，最多128个，一般是5个
         * @param sockfd    socket文件描述符
         * @param backlog   最多连接个数
         * @return          成功返回0，失败返回-1
         */
        int Listen(int sockfd, int backlog = 128);

        /*!
         * accept函数：阻塞等待客户端连接，慢速系统调用
         * @param sockfd    socket文件描述符
         * @param addr      传出参数：客户端地址信息
         * @param addelen   传入传出参数：传入addr的大小，传出客户端addr的大小，虽然我也不知道这么设计的原因是什么
         * @return          客户端socket文件描述符
         */
        int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

        /*!
         * 用于保存客户端socket信息的数据结构
         */
        class ClientSocket {
        public:
            const int sockfd;       //客户端socket文件描述符
            const std::string ip;   //客户端ip地址
            const int port;         //客户端port
            ClientSocket(int _sockfd = -1, int _port = -1, const std::string &_ip = "")
                    : sockfd(_sockfd)
                    , port(_port)
                    , ip(_ip)
            {}
        };

        /*!
         * 加强版accept函数：提取出客户端信息方便调用
         * @param sockfd        服务器socket文件描述符
         * @return              返回带有客户端socket信息的类
         */
        ClientSocket easy_accept(int sockfd);

        /*!
         * connect函数：客户端根据地址连接服务器
         * @param sockfd        客户端socket文件描述符
         * @param addr          传入参数：服务器地址信息
         * @param addrlen       addr长度
         * @return              成功0，失败-1
         */
        int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

        /*!
         * 加强版connect：根据port和ip绑定
         * @param sockfd        客户端socket文件描述符
         * @param port          服务器端口
         * @param ip            服务器ip
         * @return              成功0，失败-1
         */
        int easy_connect(int sockfd, int port, const std::string ip);

        /*!
         * 从socket中读count个字符
         * @param fd            socket文件描述符
         * @param buf           传出参数
         * @param count         读入字符个数，应该小于等于buf的长度，函数不会进行检查
         * @return              读入字符的个数
         */
        int read_n(int fd, void *buf, size_t count);

        /*!
         * 从socket中读1个字符，为了加速每次读一个块保存在局部静态变量中
         * @param fd            socket文件描述符
         * @param c             传出参数：读出的字符
         * @return              读入字符个数：0/1
         */
        int my_read(int fd, char *c);

        /*!
         * 从socket中读入一行数据，如果使用该函数必须一直使用，不能再自己使用read函数读取，原因在于有可能部分数据保存在my_read的静态buf中
         * 同getline函数一样，这个函数不会读入换行符
         * @param fd            socket文件描述符
         * @param buf           传出参数
         * @param count         buf的长度
         * @return              读入字符的个数
         */
        int read_line(int fd, void *buf, size_t count);

        /*!
         * 添加错误处理的select函数
         * @param nfds 指定被监听的文件描述符的总数，通常被设置为select所监听的所有文件描述符中的最大值加1
         * @param readfds 可读文件描述符集合,fd_set类型
         * @param writefds 可写文件描述符集合,fd_set类型
         * @param exceptfds 异常文件描述符集合,fd_set类型
         * @param timeout 用来设置select函数的超时时间，返回select等待时间，失败返回不确定。如果给两个成员都传递0则非阻塞，如果传递NULL则阻塞
         * @return 返回就绪文件描述符的总数，如果在超时时间内没有任何文件描述符就绪返回0，失败返回-1
         */
        int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

        /*!
         * 添加出错处理的poll函数
         * @param fds pollfd数组，指定所有我们感兴趣的文件描述符的事件
         * @param nfds 指定被监听事件集合fds的大小
         * @param timeout poll的超时值，单位是毫秒，为-1时，阻塞等待，为0时，非阻塞
         * @return 返回就绪文件描述符的个数，如果在超时时间内没有任何文件描述符就绪，返回0，失败返回-1
         */
        int Poll(struct pollfd *fds, nfds_t nfds, int timeout);

        /*!
         * 创建管理文件描述符的内核事件表
         * @param size 提示内核事件表需要多大
         * @return 保存内核事件表的文件描述符，用作其他epoll系统调用的第一个参数
         */
        int Epoll_create(int size);

        /*!
         * 操作epoll内核事件表
         * @param epfd 指向内核事件表的文件描述符：epoll_create的返回值
         * @param op 指定操作类型：EPOLL_CTL_ADD | EPOLL_CTL_MOD | EPOLL_CTL_DEL
         * @param fd 操作的文件描述符
         * @param event epoll_event结构指针类型，epoll_event.events指定事件类型，epoll_event.data存储用户数据
         * @return 成功返回0，失败返回-1
         */
        int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

        /*!
         * 监听epoll内核事件表上的文件描述符
         * @param epfd 指向内核事件表的文件描述符
         * @param events 用于epoll_wait返回就绪的epoll_event事件
         * @param maxevents 用于指定events的长度
         * @param timeout 超时函数，单位是毫秒，为-1时，阻塞等待，为0时，非阻塞
         * @return 返回就绪的文件描述符的个数
         */
        int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
    }
}

namespace POSIX {
    //封装一些POSIX库中的工具函数，主要是多线程库
    namespace multithreading {

        /*!
         * pthread_create函数：创建子线程
         * @param thread            传出参数：线程id
         * @param attr              传入参数：线程属性，用来设置栈的大小或者分离等
         * @param start_routine     主控函数
         * @param arg               传入参数
         * @return                  成功0，失败错误号
         */
        int Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);

        /*!
         * pthread_detach函数：将子线程设置为分离态，子线程死亡时自动释放所有资源，包括PCB，不需要回收
         * @param thread
         * @return
         */
        int Pthread_detach(pthread_t thread);

    }

}


#endif //LINUX_NET_WRAP_H
