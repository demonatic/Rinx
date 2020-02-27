# About Buffer

* #### Buffer的设计

​	由于Rinx采用是edge trigger模式的epoll，对于读操作需要一直读到返回EAGAIN下次才会触发可读事件。TcpServer每次在client_fd的read事件回调中读取完所有可读的数据，存入Inputbuf传递给上层协议处理。上层协议写入的任何数据都会送入output_buf，如果socket写满则在client_fd的可写事件回调中再次发送。

​	如图所示Buffer使用三级结构：

![](https://github.com/demonatic/Image-Hosting/blob/master/Rinx/Rinx%20Buffer.png)

* 最下层拥有一个BufferRaw基类，内部包含一个指针和指向数据长度，用于标示内存中一段连续的存储空间；BufferRaw有多种子类，如BufferFixed代表内存池中分配出的固定长度(4096字节)内存空间，BufferMalloc代表一段malloc分配器分配出来的内存空间，BufferFile代表用mmap MAP_SHARED方式在mmap区分配的一段内存空间。它们创建时都是以make_shared方式创建，以维护对BufferRaw的引用计数。

  ```c++
  class BufferRaw{
      using value_type=uint8_t;
      using Ptr=std::shared_ptr<BufferRaw>;
      
  	value_type *_p_data;
      size_t _len;
  };
  //alloc from memory pool
  template<size_t N=BufferChunkSize>
  class BufferFixed:public BufferRaw{
  public:
      BufferFixed(){
          _len=N;
          _p_data=_data;
      }
  private:
      uint8_t _data[N];
  };
  //mmap for a regular file
  class BufferFile:public BufferRaw{
  public:
      BufferFile(int fd,size_t length){
          _p_data=static_cast<value_type*>(::mmap(nullptr,length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0));
          if(_p_data==MAP_FAILED){
              throw std::bad_alloc();
          }
          _len=length;
      }
      ~BufferFile(){
          ::munmap(_p_data,_len);
      }
  };
  ```

* 中间层为BufferSlice，内部使用std::shared_ptr\<BufferRaw>共享持有一段分配出来的内存，加上start_pos和end_pos指向这段内存的其中一部分区域。只要有一个BufferSlice持有一段BufferRaw，则此BufferRaw就不会被释放。

  ```c++
  class BufferSlice{
      size_t _start_pos;
      size_t _end_pos;
      typename BufferRaw::Ptr _buf_ptr;
      
      size_t readable_size() const{ return _end_pos-_start_pos; }
      size_t writable_size() const{ return _buf_ptr->length()-_end_pos; }
  };
  ```

* 最上层为ChainBuffer，内部为一个BufferSlice的双端队列，存储了一系列BufferSlice，这些Slice可以分别指向BufferRaw的不同段。为了能够让外部看起来像一整个连续空间，实现了一个类似std::deque的迭代器，begin为第一个Slice的start_pos，end为最后一个Slice的end_pos后一个字节，遍历完一个Slice的start_pos到end_pos之间区域能够跳转到下一个Slice的start_pos位置。

  ```c++
  class ChainBuffer{
  public:
      // read data as many as possible from fd to buffer
      ssize_t read_from_fd(RxFD fd,RxReadRc &res);
      // write all data in buffer to fd(socket,file...)
      ssize_t write_to_fd(RxFD fd,RxWriteRc &res);
      // provide iterator to iterator over the content
  	read_iterator begin();
      read_iterator end();
  private:
      std::deque<BufferSlice> _buf_slice_list;
  };
  ```

  Connection中input_buffer和output_buffer均为一个ChainBuffer。读入数据到Inputbuf时，参考了muduo库中的做法在栈空间开辟了一个临时缓冲区，使用readv系统调用读一部分数据到最后一个slice的write_pos开始的区域，读的数据如果超出了最后一个Slice可写大小则超出部分数据会放到栈空间中的临时缓冲区中；然后对于超出部分的区域我们分配一个新的BufferRaw区域出来，使用一个BufferSlice指向它，并将Slice放入ChainBuffer末尾。

```c++
    char extra_buff[65535];
    std::vector<struct iovec> io_vecs(2);
    BufferSlice &tail=_buf_slice_list.back();
    io_vecs[0].iov_base=tail.write_pos();
    io_vecs[0].iov_len=tail.writable_size();
    io_vecs[1].iov_base=extra_buff;
    io_vecs[1].iov_len=sizeof extra_buff;
    ssize_t read_bytes=FDHelper::Stream::readv(fd,io_vecs,res);
```

​	 将output_buffer中的数据写到socket时使用writev系统调用。writev允许我们将所有Slice中的数据块一次发送出去，减少系统调用次数。我们得到写入的字节数后将双端队列前面所有start_pos至end_pos区域已经被完全写入的Slice出队，此时Slice持有的std::shared_ptr\<BufferRaw>析构，引用计数减一，如果引用计数置为0则会回收BufferRaw这块内存。

```c++
ssize_t ChainBuffer::write_to_fd(RxFD fd,RxWriteRc &res)
{
    std::vector<struct iovec> io_vecs;
    for(auto it_chunk=_buf_slice_list.begin();it_chunk!=_buf_slice_list.end();++it_chunk){
        BufferSlice &buf_slice=(*it_chunk);
        if((*it_chunk).readable_size()==0){
            break;
        }
        struct iovec vec;
        vec.iov_base=buf_slice.read_pos();
        vec.iov_len=buf_slice.readable_size();
        io_vecs.emplace_back(std::move(vec));
    }

    ssize_t bytes=FDHelper::Stream::writev(fd,io_vecs,res);
    if(bytes>0){
        this->commit_consume(bytes); //将双端队列前面不再需要的Slice出队
    }
    return bytes;
}
```

由于Rinx在设计之初只考虑支持HTTP这样的面向文本的协议，因此没有考虑大小端。为了能够更方便地往buffer中写入数据，使用重载+SFINAE实现了输入流操作符：

```c++
    ChainBuffer& operator<<(const char c);
    ChainBuffer& operator<<(const std::string &arg);

    template<size_t N>
    ChainBuffer &operator<<(const char (&arg)[N]){
        append(arg,N-1);
        return *this;
    }

    template<typename Arg>
    typename std::enable_if_t<
        std::is_integral_v<Arg>||std::is_floating_point_v<Arg>,ChainBuffer&> operator<<(const Arg arg)
    {
        std::string str=std::to_string(arg);
        return *this<<str;
    }

    template<typename Arg>
    typename std::enable_if_t<
        std::is_same_v<char *,std::decay_t<Arg>>||
        std::is_same_v<const char *,std::decay_t<Arg>>, ChainBuffer&>
    operator<<(const Arg arg){
        size_t str_len=strlen(arg);
        if(str_len!=0){
           append(arg,str_len);
        }
        return *this;
    }
```
* #### 这样设计的好处

  ​	首先HttpRequest类和HttpResponse类中的Body均为ChainBuffer。

  1. 作为HTTP Server发送文件时，只需往ResponseBody中添加一个Slice指向BufferFile，无需拷贝即可和ResponseHead一起直接发送出去。

  2. HTTP请求为Chunk-Encoding编码时，每块数据前都有十六进制长度和\r\n，数据块末尾也有\r\n，解析时只需改变原有Slice指针指向有效数据块，并根据需要添加新的Slice即可。如图所示，原本input_buffer中读入了两块chunk编码数据，经过HttpParser解析后改变原来Slice两个指针，使其start和end指向第一个数据块"Java"，并增添了一个新的Slice和原来的Slice共享底层的BufferRaw，新Slice指向第二个数据块Python，中间不设计涉及数据块的拷贝，只改变指针，提高效率；呈现给上层的是个逻辑上连续的HTTP body有效数据。
![](https://github.com/demonatic/Image-Hosting/blob/master/Rinx/Buffer_Chunk-Encoding.png)
  
  3. 发送HTTP Chunk-Encoding编码时，只需在HttpResponseBody前面使用BufferMalloc分配一个新的块，大小为body十六进制长度+"\r\n"两字节，令一个Slice指向整个BufferMalloc，然后prepend到HttpResponseBody开头；再append"\r\n"到body末尾(通常末尾会有剩余空间)。发送时只需要将所有Slice拷贝到output buffer末尾即可，不涉及用户body数据的拷贝或者移动，过程很廉价。
  
  ![](https://github.com/demonatic/Image-Hosting/blob/master/Rinx/buffer_send_chunk.png)