#include "FD.h"
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include "../Util/Util.h"

namespace RxFDHelper {

int Stream::create_stream() noexcept
{
    return ::socket(AF_INET,SOCK_STREAM,0);
}

bool Stream::shutdown_both(int fd) noexcept
{
    return is_open(fd)&&::shutdown(fd,SHUT_RDWR)==0;
}

bool close(int fd) noexcept
{
    return is_open(fd)&&::close(fd)==0;
}

bool is_open(int fd) noexcept
{
    return fd!=-1;
}

bool Stream::bind(int fd,const char *host,const int port) noexcept
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

bool Stream::listen(int fd) noexcept
{
    return 0==::listen(fd,SOMAXCONN);
}

int Stream::accept(int fd,RxAcceptRc &accept_res) noexcept
{
    int client_fd=-1;
    do{
        client_fd=::accept(fd,static_cast<struct sockaddr*>(nullptr),static_cast<socklen_t*>(nullptr));
        if(client_fd<0){
            switch(errno) {
                case EAGAIN:
                    accept_res=RxAcceptRc::ALL_ACCEPTED;
                break;
                case EMFILE:
                case ENFILE:
                    accept_res=RxAcceptRc::ERROR;
                break;
                default:
                    accept_res=RxAcceptRc::FAILED;
            }
        }
        else{
            accept_res=RxAcceptRc::ACCEPTING;
        }
    }while(client_fd<0&&errno==EINTR);

    return client_fd;
}

bool Stream::set_tcp_nodelay(int fd,const bool nodelay) noexcept
{
    int flags=nodelay?1:0;
    return 0==::setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&flags,sizeof(flags));
}

bool Stream::set_nonblock(int fd,const bool nonblock) noexcept
{
    return -1!=::fcntl(fd,F_SETFL,nonblock?O_NONBLOCK:O_SYNC);
}

ssize_t Stream::read(int fd, void *buffer, size_t n,RxReadRc &read_res)
{
    ssize_t total_bytes=0;
    read_res=RxReadRc::OK;

    do{
        ssize_t once_read_bytes;
        do{
            once_read_bytes=::read(fd,buffer,n);
        }while(once_read_bytes<0&&errno==EINTR);

        if(unlikely(once_read_bytes<0)){
            if(total_bytes==0){
                total_bytes=once_read_bytes;
            }
            read_res=(errno==EAGAIN)?RxReadRc::OK:RxReadRc::ERROR;
            break;
        }
        else if (unlikely(once_read_bytes==0)){
            read_res=RxReadRc::CLOSED;
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

ssize_t Stream::readv(int fd, std::vector<iovec> &io_vec,RxReadRc &read_res)
{
    ssize_t read_bytes;
    read_res=RxReadRc::OK;

    do{
        read_bytes=::readv(fd,io_vec.data(),io_vec.size());
    }while(read_bytes<0&&errno==EINTR);

    if(unlikely(read_bytes<0)){
        read_res=RxReadRc::ERROR;
    }
    else if(unlikely(read_bytes==0)){
        read_res=RxReadRc::CLOSED;
    }

    return read_bytes;
}

ssize_t Stream::write(int fd, void *buffer, size_t n, RxWriteRc &write_res)
{
    ssize_t bytes_written=0;
    write_res=RxWriteRc::OK;

    do{
        bytes_written=::write(fd,buffer,n);
    }while(bytes_written<0&&errno==EINTR);

    if(unlikely(bytes_written<0)){
        write_res=(errno==EAGAIN)?RxWriteRc::SYS_SOCK_BUFF_FULL:RxWriteRc::ERROR;
    }

    return bytes_written;
}

ssize_t Stream::writev(int fd,std::vector<struct iovec> &io_vec,RxWriteRc &write_res)
{
    ssize_t write_bytes;
    write_res=RxWriteRc::OK;

    do{
        write_bytes=::writev(fd,io_vec.data(),io_vec.size());
    }while(write_bytes<0&&errno==EINTR);

    if(unlikely(write_bytes<0)){
        write_res=(errno==EAGAIN)?RxWriteRc::SYS_SOCK_BUFF_FULL:RxWriteRc::ERROR;
    }
    return write_bytes;
}

int Event::create_event_fd() noexcept
{
    return ::eventfd(0,EFD_NONBLOCK);
}

bool Event::write_event_fd(int fd)
{
    return ::eventfd_write(fd,1)!=-1;
}

bool Event::read_event_fd(int fd){
    uint64_t data;
    int ret=::eventfd_read(fd,&data);
    return ret!=-1&&data!=0;
}

bool RegFile::open(const std::string &path,RxFD &rx_fd,bool create)
{
    int fd=::open(path.c_str(),O_RDWR|(create?O_CREAT:0));
    if(fd==-1){
       return false;
    }
    rx_fd.raw_fd=fd;
    rx_fd.fd_type=RxFD_REGULAR_FILE;
    return true;
}

long RegFile::get_file_length(RxFD fd)
{
    struct ::stat st;
    int rc=::fstat(fd.raw_fd,&st);
    return rc==-1?rc:st.st_size;
}

}
