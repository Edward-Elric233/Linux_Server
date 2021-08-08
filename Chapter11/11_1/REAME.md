# 设置connect超时
## 程序功能
设置客户端连接服务器超时时间

## 实现过程
通过`setsockopt`函数设置`sockfd`的`SO_SNDTIMEO`选项