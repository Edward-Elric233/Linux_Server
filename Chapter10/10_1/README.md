# 统一事件源
## 程序功能
信号处理函数需要尽快地执行，以确保该信号再次发送时能够及时相应。一种典型的解决方案是：

把信号的主要处理逻辑放在程序的主循环中，当信号处理函数被触发时，它只是简单地通知主循环程序接收到信号，并把信号值传递给主循环，主循环再根据接收到的信号值执行目标信号对应的逻辑代码

信号处理函数通常使用管道将信号传递给主循环：信号处理函数往管道的写端写入信号值，主循环则使用I/O复用系统调用监听管道的读端文件描述符的可读事件。

如此一来，信号事件就能和其他I/O事件一样被处理，即统一事件源

## 实现过程
在传统ET非阻塞IO复用程序中添加监听管道的读端，并设置信号的捕获函数为向管道的写端写入信号值

注意：**必须使用`sigaction`系统调用进行捕获函数的注册，并设置`sigaction.sa_flags |= SA_RESTART`**。因为使用I/O复用系统调用监听时，如果发生信号，
系统调用会被打断，返回`-1`并设置`errno`为`EINTR`，只有设置重新调用被信号终止的系统调用我们才能正常进行主循环

同时因为上面的原因，`epoll_wait`的返回值为`-1`时必须判断一下`errno`是否是`EINTR`，我就是因为忘记这里会出错，每次发送信号之后`epoll_wait`
都直接报错退出运行（因为使用`wrap.h`进行了出错封装，忘记了这回事，这说明封装系统调用时一定要小心，不能对出错处理一刀切，尤其是
出现非阻塞IO和信号之后）


## 运行结果
```shell
lsof -p ${pid}
COMMAND  PID   USER   FD      TYPE             DEVICE SIZE/OFF   NODE NAME
Server  6884 edward  cwd       DIR                8,1     4096 334030 /home/edward/SourceFile/C++/Linux_Server/cmake-build-debug
Server  6884 edward  rtd       DIR                8,1     4096      2 /
Server  6884 edward  txt       REG                8,1   352784 280018 /home/edward/SourceFile/C++/Linux_Server/cmake-build-debug/Server
Server  6884 edward  mem       REG                8,1  1369352 306246 /lib/x86_64-linux-gnu/libm-2.31.so
Server  6884 edward  mem       REG                8,1  2029224 306244 /lib/x86_64-linux-gnu/libc-2.31.so
Server  6884 edward  mem       REG                8,1   104984 266520 /lib/x86_64-linux-gnu/libgcc_s.so.1
Server  6884 edward  mem       REG                8,1  1956992 499456 /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.28
Server  6884 edward  mem       REG                8,1   157224 306257 /lib/x86_64-linux-gnu/libpthread-2.31.so
Server  6884 edward  mem       REG                8,1   191472 267233 /lib/x86_64-linux-gnu/ld-2.31.so
Server  6884 edward  DEL       REG                0,1          140249 /dev/zero
Server  6884 edward    0u      CHR              136,1      0t0      4 /dev/pts/1
Server  6884 edward    1u      CHR              136,1      0t0      4 /dev/pts/1
Server  6884 edward    2u      CHR              136,2      0t0      5 /dev/pts/2
Server  6884 edward    3u      REG                8,1     2319 274558 /home/edward/SourceFile/C++/Linux_Server/cmake-build-debug/server.log
Server  6884 edward    4u     IPv4             140250      0t0    TCP Edward:12345 (LISTEN)
Server  6884 edward    5u  a_inode               0,14        0  11424 [eventpoll]
Server  6884 edward    6u     unix 0x0000000000000000      0t0 140251 type=STREAM
Server  6884 edward    7u     unix 0x0000000000000000      0t0 140252 type=STREAM

sudo strace -p ${pid}

strace: Process 6884 attached
epoll_wait(5, [{EPOLLIN, {u32=4, u64=94102733455364}}], 1024, -1) = 1
accept(4, {sa_family=AF_INET, sin_port=htons(44376), sin_addr=inet_addr("192.168.0.112")}, [16]) = 9
stat("/etc/localtime", {st_mode=S_IFREG|0644, st_size=573, ...}) = 0
write(3, "Fri Jul 23 11:32:23 2021 : [clie"..., 66) = 66
fcntl(9, F_GETFL)                       = 0x2 (flags O_RDWR)
fcntl(9, F_SETFL, O_RDWR|O_NONBLOCK)    = 0
epoll_ctl(5, EPOLL_CTL_ADD, 9, {EPOLLIN|EPOLLET, {u32=9, u64=9}}) = 0
epoll_wait(5, [{EPOLLIN, {u32=9, u64=9}}], 1024, -1) = 1
read(9, "scasdf\n", 8191)               = 7
stat("/etc/localtime", {st_mode=S_IFREG|0644, st_size=573, ...}) = 0
write(3, "Fri Jul 23 11:32:25 2021 : [clie"..., 66) = 66
write(9, "scasdf\n", 7)                 = 7
read(9, 0x7ffe3966ab20, 8191)           = -1 EAGAIN (资源暂时不可用)
stat("/etc/localtime", {st_mode=S_IFREG|0644, st_size=573, ...}) = 0
write(3, "Fri Jul 23 11:32:25 2021 : [clie"..., 67) = 67
epoll_wait(5, [{EPOLLIN, {u32=9, u64=9}}], 1024, -1) = 1
read(9, "", 8191)                       = 0
epoll_ctl(5, EPOLL_CTL_DEL, 9, 0x7ffe3966a99c) = 0
stat("/etc/localtime", {st_mode=S_IFREG|0644, st_size=573, ...}) = 0
write(3, "Fri Jul 23 11:32:29 2021 : [clie"..., 67) = 67
epoll_wait(5, 0x5596afa8b160, 1024, -1) = -1 EINTR (被中断的系统调用)
--- SIGINT {si_signo=SIGINT, si_code=SI_USER, si_pid=7072, si_uid=1000} ---
write(7, "\2", 1)                       = 1
rt_sigreturn({mask=[QUIT]})             = -1 EINTR (被中断的系统调用)
epoll_wait(5, [{EPOLLIN, {u32=6, u64=6}}], 1024, -1) = 1
read(6, "\2", 8191)                     = 1
stat("/etc/localtime", {st_mode=S_IFREG|0644, st_size=573, ...}) = 0
write(3, "Fri Jul 23 11:32:37 2021 : [sign"..., 51) = 51
close(4)                                = 0
close(6)                                = 0
close(7)                                = 0
stat("/etc/localtime", {st_mode=S_IFREG|0644, st_size=573, ...}) = 0
write(3, "Fri Jul 23 11:32:37 2021 : [EXIT"..., 34) = 34
close(3)                                = 0
munmap(0x7f36cdda1000, 32)              = 0
exit_group(1)                           = ?
+++ exited with 1 +++
```
