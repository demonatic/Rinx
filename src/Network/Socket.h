#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <vector>

enum class Rx_Accept_Res{
    ACCEPTING,
    ALL_ACCEPTED,
    FAILED,
    ERROR
};

enum class Rx_Read_Res{
    OK,
    CLOSED,
    ERROR,
};

enum class Rx_Write_Res{
    OK,
    SOCK_BUFF_FULL,
    ERROR
};

class RxSock
{
public:
    RxSock()=delete;

    static int create() noexcept;
    static bool shutdown_both(int fd) noexcept;
    static bool close(int fd) noexcept;
    static bool is_open(int fd) noexcept;

    static bool bind(int fd,const char *host,const int port) noexcept;
    static bool listen(int fd) noexcept;
    static int accept(int fd,Rx_Accept_Res &accept_res) noexcept;

    static bool set_tcp_nodelay(int fd,const bool nodelay) noexcept;
    static bool set_nonblock(int fd,const bool nonblock) noexcept;

    static ssize_t read(int fd,void *buffer,size_t n,Rx_Read_Res &read_res);
    static ssize_t readv(int fd,std::vector<struct iovec> &io_vec,Rx_Read_Res &read_res);

    static ssize_t write(int fd,void *buffer,size_t n,Rx_Write_Res &write_res);
    static ssize_t writev(int fd,std::vector<struct iovec> &io_vec,Rx_Write_Res &write_res);
};

#endif // SOCKET_H
