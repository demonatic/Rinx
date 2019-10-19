#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/eventfd.h>
#include <vector>

enum class RxAcceptRc{
    ACCEPTING,
    ALL_ACCEPTED,
    FAILED,
    ERROR
};

enum class RxReadRc{
    OK,
    CLOSED,
    ERROR,
};

enum class RxWriteRc{
    OK,
    SYS_SOCK_BUFF_FULL,
    ERROR
};

class RxSock
{
public:
    RxSock()=delete;

    ///for stream
    static int create_stream() noexcept;

    static bool shutdown_both(int fd) noexcept;
    static bool close(int fd) noexcept;
    static bool is_open(int fd) noexcept;

    static bool bind(int fd,const char *host,const int port) noexcept;
    static bool listen(int fd) noexcept;
    static int accept(int fd,RxAcceptRc &accept_res) noexcept;

    static bool set_tcp_nodelay(int fd,const bool nodelay) noexcept;
    static bool set_nonblock(int fd,const bool nonblock) noexcept;

    static ssize_t read(int fd,void *buffer,size_t n,RxReadRc &read_res);
    static ssize_t readv(int fd,std::vector<struct iovec> &io_vec,RxReadRc &read_res);

    static ssize_t write(int fd,void *buffer,size_t n,RxWriteRc &write_res);
    static ssize_t writev(int fd,std::vector<struct iovec> &io_vec,RxWriteRc &write_res);

    ///for event fd
    static int create_event_fd() noexcept;
    static bool read_event_fd(int fd);
    static bool write_event_fd(int fd);
};



#endif // SOCKET_H
