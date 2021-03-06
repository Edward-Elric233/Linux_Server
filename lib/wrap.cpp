//
// Created by edward on 2021/6/3.
//

#include "wrap.h"

namespace C_std {

    ssize_t Read(int fd, void *buf, size_t count) {
        AGAIN:
        int n = read(fd, buf, count);
        if (n < 0) {
            //出错处理
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                //非阻塞调用，让程序进行处理
            } else if (errno == EINTR) {
                //慢速系统调用，如果被注册捕捉函数的信号中断则重新读取
                goto AGAIN;
            } else {
                perror_exit("read error");
            }
        }
        return n;
    }

    ssize_t Write(int fd, const void *buf, size_t count) {
        AGAIN:
        int n = write(fd, buf, count);
        if (n < 0) {
            //出错处理
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
            } else if (errno == EINTR) {
                //慢速系统调用，如果被注册捕捉函数的信号中断则重新读取
                goto AGAIN;
            } else {
                perror_exit("write error");
            }
        }
        return n;
    }

    int Open(const char *pathname, int flags, mode_t mode) {
        return check_error(open(pathname, flags, mode), "open error");
    }

    int Close(int fd) {
        return check_error(close(fd));
    }


    int read_n(int fd, void *buf, size_t count) {
        int n_left = count;                         //必须是int类型，因为后面会检查n_left <= 0，如果是size_t则只能检查==0
        int n_read = 0;
        char *p = reinterpret_cast<char *>(buf);    //如果不转换成char *类型则无法进行算数运算
        while (n_left > 0) {
            n_read = Read(fd, p, n_left);
            if (n_read == 0) {
                //非常重要，有可能对端关闭，退出read
                break;
            }
            n_left -= n_read;
            p += n_read;
        }
        return count - n_left;
    }

    int my_read(int fd, char *c) {
        constexpr int BUFFSIZ = 100;
        static char buf[BUFFSIZ];
        static int buf_size = 0;
        static int idx = 0;
        if (buf_size <= 0) {
            buf_size = Read(fd, buf, BUFFSIZ);
            idx = 0;
            if (buf_size == 0) {
                //读入失败
                return 0;
            }
        }
        *c = buf[idx];          //拷贝值，不能让指针指向该静态区域
        ++idx;
        --buf_size;
        return 1;
    }

    int read_line(int fd, void *buf, size_t count) {
        char c;
        int n_read = 0;
        int ret = 0;
        char *p = reinterpret_cast<char *>(buf);
//        --count;                //因为最后需要添加一个字符串结束符，socket中没有，但是原本就没有，为了统一没有就没有吧
        while (n_read < count) {
            ret = my_read(fd, &c);
            if (ret == 0 || c == '\n') {
                break;
            }
            p[n_read] = c;
            ++n_read;
        }
        return n_read;
    }

    namespace IPC {
        void *Mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
            void *p = mmap(addr, length, prot, flags, fd, offset);
            if (p == MAP_FAILED) {
                perror_exit("mmap error");
            }
            return p;
        }

        int Munmap(void *addr, size_t length) {
            return check_error(munmap(addr, length), "munmap error");
        }
    }

    namespace Network {

        int Socket(int domain, int type, int protocol) {
            return check_error(socket(domain, type, protocol), "socket error");
        }

        void port_reuse(int sockfd) {
            int opt = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        }

        int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
            return check_error(bind(sockfd, addr, addrlen), "bind error");
        }

        int easy_bind(int sockfd, int port, const std::string &ip) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);                //需要转换成网络字节序：大端模式
            //如果ip为127.0.0.1，则等价于addr.sin_addr.s_addr = htol(INADDR_ANY);
            int ret = inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
            if (ret == 0) {
                perror_exit("ip convert error");    //src不包含代表指定地址族中有效网络地址的字符串
            } else if(ret == -1){
                perror_exit("af to ip error");            //af不包含有效的地址族
            }
            return Bind(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
        }

        int Listen(int sockfd, int backlog) {
            return check_error(listen(sockfd, backlog), "listen error");
        }


        int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
            AGAIN:
            int cfd = accept(sockfd, addr, addrlen);
            if (cfd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    //套接字被标记为非阻塞，并且不存在可接受的连接。虽然我不太了解套接字标记为非阻塞是什么意思。。。
                    goto AGAIN;
                } else if (errno == EINTR) {
                    //慢速系统调被注册捕捉函数的信号打断，则重新accept
                    goto AGAIN;
                } else {
                    perror_exit("accept error");
                }
            }
            return cfd;
        }

        ClientSocket easy_accept(int sockfd) {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            memset(&addr, 0, sizeof(addr));
            int cfd = Accept(sockfd, reinterpret_cast<struct sockaddr *>(&addr), &addr_len);
            char buf[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &addr.sin_addr.s_addr, buf, INET_ADDRSTRLEN) == nullptr) {
                perror_exit("af to ip error");
            }
            return ClientSocket(cfd, ntohs(addr.sin_port), buf);
        }

        int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
            return check_error(connect(sockfd, addr, addrlen), "connect error");
        }

        int easy_connect(int sockfd, int port, const std::string ip) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            int ret = inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
            if (ret == 0) {
                perror_exit("ip convert error");    //src不包含代表指定地址族中有效网络地址的字符串
            } else if (ret == -1){
                perror_exit("af to ip error");            //af不包含有效的地址族
            }
            return Connect(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
        }

        int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
            return check_error(select(nfds, readfds, writefds, exceptfds, timeout), "select error");
        }

        int Poll(struct pollfd *fds, nfds_t nfds, int timeout) {
            return check_error(poll(fds, nfds, timeout), "poll error");
        }

        int Epoll_create(int size) {
            return check_error(epoll_create(size), "epoll_create error");
        }

        int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
            return check_error(epoll_ctl(epfd, op, fd, event), "epoll_ctl error");
        }

        int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
            int ret = epoll_wait(epfd, events, maxevents, timeout);
            if (ret == -1) {
                if (errno = EINTR) {
                    return -1;
                } else {
                    perror_exit("epoll_wait error");
                }
            }
            return ret;
        }

    }

}
namespace POSIX {
    //封装一些POSIX库中的工具函数，主要是多线程库
    namespace multithreading {

        int Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
            return check_error(pthread_create(thread, attr, start_routine, arg), "pthrad_create error");
        }

        int Pthread_detach(pthread_t thread) {
            return check_error(pthread_detach(thread));
        }

    }

}
