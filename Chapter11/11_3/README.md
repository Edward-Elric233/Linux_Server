# 处理非活动连接
## 程序功能
对每个客户端连接设置一个定时器，如果客户端超过30s未发送数据则认为该客户端是非活动连接，服务器会自动断开连接。
服务器每隔10s检查一次定时器集合，处理非活动连接

## 实现过程
程序基本的框架仍然是使用`epoll ET`的非阻塞IO多路复用服务器，同时统一事件源，在主循环中处理信号，通过setitimer函数设置定时周期

主要是为了熟悉API，在没有写之前对很多API都有点陌生了，因为一直使用的是自己封装好的

通过自己手动实现一遍，对`epoll`函数族尤其是`epoll_ctl`函数、对设置文件描述符非阻塞的`fcntl`函数、设置端口复用的`setsockopt`函数、
注册信号捕捉的`sigaction`函数、设置定时器的`setitimer`函数，创建本地连接的`socketpair`函数都有了更深刻的理解

程序设计的难点在于定时器的设计

对于客户端信息的保存使用一个类`ClientData`保存IP、端口、文件描述符，以及对应的定时器迭代器，使用`unordered_map`保存从文件描述符到客户端信息的映射

定时器类`Timer`保存定时期限、回调函数、客户端数据，回调函数会调用客户端数据，这里回调函数的传入参数如果是客户端数据会更好，但是我这里比较简单，而且也不是写框架就直接写成`sockfd`了
使用`set`保存定时器集合，使用`set`是经过深思熟虑的，其实觉得在这个程序中花费精力最多的就是这个数据结构的选择上。

书中这个数据结构选择的是单向链表，而且还是自己实现的，因为没有用哑节点，导致插入删除等操作变得非常的复杂。

首先来看这个数据结构会做哪些操作：
- 有序
- 随机访问、修改、删除
- 从头部删除

如果使用链表的话，为了保持链表的有序，每次修改、删除（随机访问可以通过指针进行）的复杂度为O(n)，其他都是O(1)

如果使用`set`的话，有序是自动满足的，每次访问、修改、删除的复杂度都是O(logn)

当连接更多时，显然是`set`更有优势，而且使用STL中的`set`实现起来也更简单，需要注意的是**在遍历`set`的时候如果有删除操作，一定要使用后置`++`保证在删除后遍历的迭代器仍然有效**

另一个值得一提的地方是为了方便对信号的捕捉、处理，程序使用`unordered_map`保存了信号到处理函数的映射关系，使用了`lambda`表达式和`function`类型，C++11yyds