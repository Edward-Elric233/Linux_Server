# 监听socket测试程序
## 程序功能
**通过设置`listen`函数创建一个监听队列以存放待处理的客户连接，`backlog`参数设置内核监听队列的最大长度，如果监听队列的长度超过backlog，服务器将不
受理新的客户连接。**

在此之前，我以为`listen`函数阶段并不能建立连接，然而通过测试发现，`listen`函数阶段已经可以建立连接了。
## 实现过程
使用了自己以前封装的`wrap`网络库，添加了出错处理

然后使用两台主机进行测试，在使用`telnet`命令连接服务器的时候客户端总是报错：`telnet: Unable to connect to remote host: Connection refused`

经过使用`wireshark`进行抓包，发现客户端发送的TCP请求发送到了服务器，但是服务器端说找不到客户端，我以为是客户端的防火墙的问题，结果关闭客户端的防
火墙后发现仍旧不行（因为客户端是我新安装的Linux系统），最后才发现是服务器防火墙的问题，输入如下命令后解决该问题：
```shell
sudo iptables -P INPUT ACCEPT
sudo iptables -P FORWARD ACCEPT
sudo iptables -P OUTPUT ACCEPT
sudo iptables -F
```

## 实现结果
发现`listen`后客户端最多可以和服务器建立`backlog + 1`个连接，再多的话服务端并没有出现像书中说的`SYN_RECV`状态，客户端出现了`SYN_SENT`状态。
的确我也认为`SYN_RECV`状态的出现不太合理，因为如果已经`SYN_RECV`了，则应该建立连接了。