# 使用Socket进行通信
## 程序功能
客户端使用`connect`函数主动与服务器建立连接，使用`shutdown`函数关闭连接，使用`recv`和`send`函数进行数据的收发

如果`recv`函数返回0，则意味着通信对方已经关闭连接

通过设置`send`和`recv`函数的参数，我们可以对数据的属性进行设置，例如可以发送紧急数据

## 实现过程
服务端`Server`和客户端`Client`都是普通的流程

当时实现的时候`Server`没有什么问题，但是`Client`总是出错，看来看去，才发现我在`Client`中使用了`listen`函数

## 实现结果
带外（紧急）数据最多只能有一个字符，并且会截断正常的数据