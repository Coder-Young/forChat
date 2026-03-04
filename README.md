# forChat
实现一个简易的聊天软件，包括PC端QT界面编程，asio异步服务器设计，beast网络库搭建http网关，nodejs搭建验证服务，各服务间用grpc通信，server和client用asio通信等，包括用户信息的录入等。实现跨平台设计，之后考虑移植到windows中。
## 架构设计
[架构](https://github.com/Coder-Young/forChat/blob/main/pic/1709009717000.jpg)
GateServer为网关服务，主要应对客户端的连接和注册请求，因为服务器是是分布式，所以GateServer收到用户连接请求后会查询状态服务选择一个负载较小的Server地址给客户端，客户端拿着这个地址直接和Server通信建立长连接。
当用户注册时会发送给GateServer, GateServer调用VarifyServer验证注册的合理性并发送验证码给客户端，客户端拿着这个验证码去GateServer注册即可。
StatusServer， ServerA， ServerB都可以直接访问Redis和Mysql服务。
