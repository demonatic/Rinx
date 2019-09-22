#include "Socket.h"
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <errno.h>
#include <assert.h>
#include "../Util/Util.h"

int RxSock::create_stream() noexcept
{
    return ::socket(AF_INET,SOCK_STREAM,0);
}

bool RxSock::shutdown_both(int fd) noexcept
{
    return is_open(fd)&&::shutdown(fd,SHUT_RDWR)==0;
}

bool RxSock::close(int fd) noexcept
{
    return is_open(fd)&&::close(fd)==0;
}

bool RxSock::is_open(int fd) noexcept
{
    return fd!=-1;
}

bool RxSock::bind(int fd,const char *host,const int port) noexcept
{
    struct sockaddr_in sock_addr{
        AF_INET,
        htons(static_cast<uint16_t>(port)),
        {htonl(INADDR_ANY)},
        {0}
    };
    if(host&&1!=inet_pton(AF_INET,host,&(sock_addr.sin_addr))){
       return false;
    }
//    bool dont_linger=true;
//    ::setsockopt(fd,SOL_SOCKET,SO_LINGER,&(dont_linger),sizeof(dont_linger));
    return 0==::bind(fd,reinterpret_cast<const ::sockaddr*>(&sock_addr),sizeof(::sockaddr_in));
}

bool RxSock::listen(int fd) noexcept
{
    return 0==::listen(fd,SOMAXCONN);
}

int RxSock::accept(int fd,Rx_Accept_Res &accept_res) noexcept
{
    int client_fd=-1;
    do{
        client_fd=::accept(fd,static_cast<struct sockaddr*>(nullptr),static_cast<socklen_t*>(nullptr));
        if(client_fd<0){
            switch(errno) {
                case EAGAIN:
                    accept_res=Rx_Accept_Res::ALL_ACCEPTED;
                break;
                case EMFILE:
                case ENFILE:
                    accept_res=Rx_Accept_Res::ERROR;
                break;
                default:
                    accept_res=Rx_Accept_Res::FAILED;
            }
        }
        else{
            accept_res=Rx_Accept_Res::ACCEPTING;
        }
    }while(client_fd<0&&errno==EINTR);

    return client_fd;
}

bool RxSock::set_tcp_nodelay(int fd,const bool nodelay) noexcept
{
    int flags=nodelay?1:0;
    return 0==::setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&flags,sizeof(flags));
}

bool RxSock::set_nonblock(int fd,const bool nonblock) noexcept
{
    return -1!=::fcntl(fd,F_SETFL,nonblock?O_NONBLOCK:O_SYNC);
}

ssize_t RxSock::read(int fd, void *buffer, size_t n,Rx_Read_Res &read_res)
{
    ssize_t total_bytes=0;
    read_res=Rx_Read_Res::OK;

    do{
        ssize_t once_read_bytes;
        do{
            once_read_bytes=::read(fd,buffer,n);
        }while(once_read_bytes<0&&errno==EINTR);

        if(unlikely(once_read_bytes<0)){
            if(total_bytes==0){
                total_bytes=once_read_bytes;
            }
            read_res=(errno==EAGAIN)?Rx_Read_Res::OK:Rx_Read_Res::ERROR;
            break;
        }
        else if (unlikely(once_read_bytes==0)){
            read_res=Rx_Read_Res::CLOSED;
            break;
        }
        else{
            total_bytes+=once_read_bytes;
            if(total_bytes>=n){
                break;
            }
        }
    }while(true);

    return total_bytes;
}

ssize_t RxSock::readv(int fd, std::vector<iovec> &io_vec,Rx_Read_Res &read_res)
{
    ssize_t read_bytes;
    read_res=Rx_Read_Res::OK;

    do{
        read_bytes=::readv(fd,io_vec.data(),io_vec.size());
    }while(read_bytes<0&&errno==EINTR);

    if(unlikely(read_bytes<0)){
        read_res=Rx_Read_Res::ERROR;
    }
    else if(unlikely(read_bytes==0)){
        read_res=Rx_Read_Res::CLOSED;
    }

    return read_bytes;
}

ssize_t RxSock::write(int fd, void *buffer, size_t n, Rx_Write_Res &write_res)
{
    ssize_t bytes_written=0;
    write_res=Rx_Write_Res::OK;

    do{
        bytes_written=::write(fd,buffer,n);
    }while(bytes_written<0&&errno==EINTR);

    if(unlikely(bytes_written<0)){
        write_res=(errno==EAGAIN)?Rx_Write_Res::SOCK_BUFF_FULL:Rx_Write_Res::ERROR;
    }

    return bytes_written;
}

ssize_t RxSock::writev(int fd,std::vector<struct iovec> &io_vec,Rx_Write_Res &write_res)
{
    ssize_t write_bytes;
    write_res=Rx_Write_Res::OK;

    do{
        write_bytes=::writev(fd,io_vec.data(),io_vec.size());
    }while(write_bytes<0&&errno==EINTR);

    if(unlikely(write_bytes<0)){
        write_res=(errno==EAGAIN)?Rx_Write_Res::SOCK_BUFF_FULL:Rx_Write_Res::ERROR;
    }
    return write_bytes;
}

int RxSock::create_event_fd() noexcept
{
    return ::eventfd(0,EFD_NONBLOCK);
}

bool RxSock::write_event_fd(int fd)
{
    return ::eventfd_write(fd,1)!=-1;
}

bool RxSock::read_event_fd(int fd){
    uint64_t data;
    int ret=::eventfd_read(fd,&data);
    return ret!=-1&&data!=0;
}
