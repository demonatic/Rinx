# Buffer设计

* #### 最初的实现

​	由于Rinx采用是edge trigger模式的epoll，对于读操作需要一直读到返回EAGAIN下次才会触发可读事件，