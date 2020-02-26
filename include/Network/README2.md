# Buffer设计

* #### 最初的实现

​	由于Rinx采用是edge trigger模式的epoll，对于读操作需要一直读到返回EAGAIN下次才会触发可读事件。TcpServer每次在client_fd的read事件回调中读取完所有可读的数据，存入Inputbuf传递给上层协议处理。

​	Buffer使用三级结构，最下层是BufferRaw，内部包含一个指针和指向数据长度，用于标示内存中一段连续的存储空间；中间层是

