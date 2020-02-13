#include "Network/FD.h"
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "Util/Util.h"

namespace Rinx {
namespace FDHelper {

RxFD Stream::create_client_stream() noexcept
{
    int fd=::socket(AF_INET,SOCK_STREAM,0);
    return RxFD(fd!=-1?RxFDType::FD_CLIENT_STREAM:RxFDType::FD_INVALID,fd);
}

RxFD Stream::create_serv_sock() noexcept
{
    int fd=::socket(AF_INET,SOCK_STREAM,0);
    return RxFD(fd!=-1?RxFDType::FD_LISTEN:RxFDType::FD_INVALID,fd);
}

bool Stream::shutdown_both(RxFD fd) noexcept
{
    return is_open(fd)&&::shutdown(fd.raw,SHUT_RDWR)==0;
}

bool close(RxFD &fd) noexcept
{
    bool ok=false;
    int t_raw=fd.raw;
    if(is_open(fd)){
        fd=RxInvalidFD;
        ok=::close(t_raw)==0;
    }
    return ok;
}

bool is_open(RxFD fd) noexcept
{
    return fd!=RxInvalidFD;
}

bool Stream::bind(RxFD fd,const char *host,const int port) noexcept
{
    assert(fd.type==RxFDType::FD_LISTEN);
    struct sockaddr_in sock_addr{
        AF_INET,
        htons(static_cast<uint16_t>(port)),
        {htonl(INADDR_ANY)},
        {0}
    };
    if(host&&1!=inet_pton(AF_INET,host,&(sock_addr.sin_addr))){
        return false;
    }
    return 0==::bind(fd.raw,reinterpret_cast<const ::sockaddr*>(&sock_addr),sizeof(::sockaddr_in));
}

bool Stream::listen(RxFD fd) noexcept
{
    assert(fd.type==RxFDType::FD_LISTEN);
    return 0==::listen(fd.raw,SOMAXCONN);
}

RxFD Stream::accept(RxFD fd,RxAcceptRc &accept_res) noexcept
{
    assert(fd.type==RxFDType::FD_LISTEN);
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

    return RxFD(RxFDType::FD_CLIENT_STREAM,client_fd);
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
    ssize_t n_read=0;
    read_res=RxReadRc::OK;

    do{
        ssize_t ret;
        do{
            ret=::read(fd.raw,static_cast<uint8_t*>(buffer)+n_read,n-n_read);
        }while(ret<0&&errno==EINTR);

        if(ret>0){
            n_read+=ret;
        }
        else if(ret<0){
            read_res=(errno==EAGAIN)?RxReadRc::OK:RxReadRc::ERROR;
            break;
        }
        else if (ret==0){
            read_res=RxReadRc::CLOSED;
            break;
        }
    }while(true);

    return n_read;
}

ssize_t Stream::readv(RxFD fd, std::vector<iovec> &io_vec,RxReadRc &read_res)
{
    ssize_t read_bytes;
    read_res=RxReadRc::OK;
    do{
        read_bytes=::readv(fd.raw,io_vec.data(),io_vec.size());
    }while(read_bytes<0&&errno==EINTR);

    if(read_bytes<0){
        read_res=(errno==EAGAIN?RxReadRc::SOCK_RD_BUFF_EMPTY:RxReadRc::ERROR);
    }
    else if(unlikely(read_bytes==0)){
        read_res=RxReadRc::CLOSED;
    }
    return read_bytes;
}

ssize_t Stream::write(RxFD fd, void *buffer, size_t n, RxWriteRc &write_res)
{
    ssize_t n_write=0;
    write_res=RxWriteRc::OK;

    while(true){
        ssize_t ret;
        do{
            ret=::write(fd.raw,static_cast<uint8_t*>(buffer)+n_write,n-n_write);
        }while(ret<0&&errno==EINTR);

        if(ret>0){
            n_write+=ret;
        }
        else if(n_write<0){
            write_res=(errno==EAGAIN)?RxWriteRc::SOCK_SD_BUFF_FULL:RxWriteRc::ERROR;
            break;
        }
    }
    return n_write;
}

ssize_t Stream::writev(RxFD fd,std::vector<struct iovec> &io_vec,RxWriteRc &write_res)
{
    ssize_t write_bytes;
    write_res=RxWriteRc::OK;
    do{
        write_bytes=::writev(fd.raw,io_vec.data(),io_vec.size());
    }while(write_bytes<0&&errno==EINTR);

    if(write_bytes<0){
        write_res=(errno==EAGAIN)?RxWriteRc::SOCK_SD_BUFF_FULL:RxWriteRc::ERROR;
    }
    return write_bytes;
}

RxFD Event::create_event_fd() noexcept
{
    int fd=::eventfd(0,EFD_NONBLOCK);
    return RxFD(fd!=-1?RxFDType::FD_EVENT:RxFDType::FD_INVALID,fd);
}

bool Event::write_event_fd(RxFD fd)
{
    assert(fd.type==FD_EVENT);
    return ::eventfd_write(fd.raw,1)!=-1;
}

bool Event::read_event_fd(RxFD fd){
    assert(fd.type==FD_EVENT);
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
    fd=RxFD(FD_REGULAR_FILE,raw_fd);
    return true;
}

long RegFile::get_file_length(RxFD fd)
{
    assert(fd.type==FD_REGULAR_FILE);
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
    FDHelper::close(_file_fd);
}

File::File(File &&other) noexcept
{
    this->_file_fd=other._file_fd;
    other._file_fd=RxInvalidFD;
}

File &File::operator==(File &&other) noexcept
{
    if(this!=&other){
        FDHelper::close(this->_file_fd);
        this->_file_fd=other._file_fd;
        other._file_fd=RxInvalidFD;
    }
    return *this;
}

long File::get_len() const
{
    return RegFile::get_file_length(_file_fd);
}

RxFD File::get_fd() const{
    return _file_fd;
}

} //namespace Rinx
}
