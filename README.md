# foolsock
foolsock是为了实现RPC通信中TCP长连接而开发的PHP扩展

### 简介
很多项目随着业务规模的增长(尤其是终端众多的情况下)逐渐向服务化演变，常见的一种架构模型是将相对独立的业务抽象为单独的服务(如用户模块)，具体的业务层(如:网页端、移动端)来调用各个服务，这种架构大大降低了各业务之间的耦合度，同时最大限度的提高了模块的重用性。
(https://github.com/pangudashu/foolsock/blob/master/image/p_1.jpg)

