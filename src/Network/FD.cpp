#include "FD.h"
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "../Util/Util.h"

namespace RxFDHelper {

RxFD Stream::create_client_stream() noexcept
{
    return RxFD(RxFDType::RxFD_CLIENT_STREAM,::socket(AF_INET,SOCK_STREAM,0));
}

RxFD Stream::create_serv_sock() noexcept
{
    return RxFD(RxFDType::RxFD_LISTEN,::socket(AF_INET,SOCK_STREAM,0));
}

bool Stream::shutdown_both(RxFD fd) noexcept
{
    return is_open(fd)&&::shutdown(fd.raw,SHUT_RDWR)==0;
}

bool close(RxFD &fd) noexcept
{
    if(is_open(fd)){
        int raw=fd.raw;
        fd=RxInvalidFD;
        return ::close(raw)==0;
    }
    return false;
}

bool is_open(RxFD fd) noexcept
{
    return fd!=RxInvalidFD;
}

bool Stream::bind(RxFD fd,const char *host,const int port) noexcept
{
    assert(fd.type==RxFDType::RxFD_LISTEN);
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
    return 0==::bind(fd.raw,reinterpret_cast<const ::sockaddr*>(&sock_addr),sizeof(::sockaddr_in));
}

bool Stream::listen(RxFD fd) noexcept
{
    assert(fd.type==RxFDType::RxFD_LISTEN);
    return 0==::listen(fd.raw,SOMAXCONN);
}

RxFD Stream::accept(RxFD fd,RxAcceptRc &accept_res) noexcept
{
    assert(fd.type==RxFDType::RxFD_LISTEN);
    int client_fd=-1;
    do{
        client_fd=::accept(fd.raw,static_cast<struct sockaddr*>(nullptr),static_cast<socklen_t*>(nullptr));
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

    return RxFD(RxFDType::RxFD_CLIENT_STREAM,client_fd);
}

bool Stream::set_tcp_nodelay(RxFD fd,const bool nodelay) noexcept
{
    int flags=nodelay?1:0;
    return 0==::setsockopt(fd.raw,IPPROTO_TCP,TCP_NODELAY,&flags,sizeof(flags));
}

bool Stream::set_nonblock(RxFD fd,const bool nonblock) noexcept
{
    return -1!=::fcntl(fd.raw,F_SETFL,nonblock?O_NONBLOCK:O_SYNC);
}

ssize_t Stream::read(RxFD fd, void *buffer, size_t n,RxReadRc &read_res)
{
    ssize_t total_bytes=0;
    read_res=RxReadRc::OK;

    do{
        ssize_t once_read_bytes;
        do{
            once_read_bytes=::read(fd.raw,buffer,n);
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

ssize_t Stream::readv(RxFD fd, std::vector<iovec> &io_vec,RxReadRc &read_res)
{
    ssize_t read_bytes;
    read_res=RxReadRc::OK;

    do{
        read_bytes=::readv(fd.raw,io_vec.data(),io_vec.size());
    }while(read_bytes<0&&errno==EINTR);

    if(unlikely(read_bytes<0)){
        read_res=RxReadRc::ERROR;
    }
    else if(unlikely(read_bytes==0)){
        read_res=RxReadRc::CLOSED;
    }

    return read_bytes;
}

ssize_t Stream::write(RxFD fd, void *buffer, size_t n, RxWriteRc &write_res)
{
    ssize_t bytes_written=0;
    write_res=RxWriteRc::OK;

    do{
        bytes_written=::write(fd.raw,buffer,n);
    }while(bytes_written<0&&errno==EINTR);

    if(unlikely(bytes_written<0)){
        write_res=(errno==EAGAIN)?RxWriteRc::SYS_SOCK_BUFF_FULL:RxWriteRc::ERROR;
    }

    return bytes_written;
}

ssize_t Stream::writev(RxFD fd,std::vector<struct iovec> &io_vec,RxWriteRc &write_res)
{
    ssize_t write_bytes;
    write_res=RxWriteRc::OK;

    do{
        write_bytes=::writev(fd.raw,io_vec.data(),io_vec.size());
    }while(write_bytes<0&&errno==EINTR);

    if(unlikely(write_bytes<0)){
        write_res=(errno==EAGAIN)?RxWriteRc::SYS_SOCK_BUFF_FULL:RxWriteRc::ERROR;
    }
    return write_bytes;
}

RxFD Event::create_event_fd() noexcept
{
    return RxFD(RxFDType::RxFD_EVENT,::eventfd(0,EFD_NONBLOCK));
}

bool Event::write_event_fd(RxFD fd)
{
    assert(fd.type==RxFD_EVENT);
    return ::eventfd_write(fd.raw,1)!=-1;
}

bool Event::read_event_fd(RxFD fd){
    assert(fd.type==RxFD_EVENT);
    uint64_t data;
    int ret=::eventfd_read(fd.raw,&data);
    return ret!=-1&&data!=0;
}

bool RegFile::open(const std::string &path,RxFD &fd,bool create)
{
    int raw_fd=::open(path.c_str(),O_RDWR|(create?O_CREAT:0));
    if(raw_fd==-1){
       return false;
    }
    fd=RxFD(RxFD_REGULAR_FILE,raw_fd);
    return true;
}

long RegFile::get_file_length(RxFD fd)
{
    assert(fd.type==RxFD_REGULAR_FILE);
    struct ::stat st;
    int rc=::fstat(fd.raw,&st);
    return rc==-1?rc:st.st_size;
}

File::File(const std::string &path,bool create):_file_fd(RxInvalidFD)
{
    if(!RegFile::open(path,_file_fd,create)){
        throw std::runtime_error("error occur when open file "+path+' '+strerror(errno));
    }
}

File::~File(){
    if(_file_fd!=RxInvalidFD){
        RxFDHelper::close(_file_fd);
    }
}

long File::get_len() const
{
    return RegFile::get_file_length(_file_fd);
}

RxFD File::get_fd() const{
    return _file_fd;
}

}
