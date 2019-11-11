#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/eventfd.h>
#include <vector>
#include <string>

enum RxFDType:uint32_t{
    RxFD_LISTEN=0,
    RxFD_CLIENT_STREAM,
    RxFD_EVENT,
    RxFD_REGULAR_FILE,
    RxFD_INVALID,

    __RxFD_TYPE_COUNT,
};

struct RxFD{
    RxFDType fd_type;
    int raw_fd;
    bool operator==(const RxFD &other){
        return this->fd_type==other.fd_type&&this->raw_fd==other.raw_fd;
    }
    bool operator!=(const RxFD &other){
        return !((*this)==other);
    }
};

constexpr static RxFD InvalidRxFD={RxFDType::RxFD_INVALID,-1};

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

namespace RxFDHelper{

    /// @brief close the fd and reset fd to -1
    /// @return whether the close operation is executed successfully
    bool close(int &fd) noexcept;

    /// @brief check whether the fd is -1
    bool is_open(int fd) noexcept;

    namespace Stream{
        int create_stream() noexcept;

        bool shutdown_both(int fd) noexcept;

        bool bind(int fd,const char *host,const int port) noexcept;
        bool listen(int fd) noexcept;
        int accept(int fd,RxAcceptRc &accept_res) noexcept;

        bool set_tcp_nodelay(int fd,const bool nodelay) noexcept;
        bool set_nonblock(int fd,const bool nonblock) noexcept;

        ssize_t read(int fd,void *buffer,size_t n,RxReadRc &read_res);
        ssize_t readv(int fd,std::vector<struct iovec> &io_vec,RxReadRc &read_res);

        ssize_t write(int fd,void *buffer,size_t n,RxWriteRc &write_res);
        ssize_t writev(int fd,std::vector<struct iovec> &io_vec,RxWriteRc &write_res);

    };

    namespace Event{
        int create_event_fd() noexcept;

        bool read_event_fd(int fd);
        bool write_event_fd(int fd);
    }


    namespace RegFile{
        /// @return if the file is opened successfully
        bool open(const std::string &path,RxFD &rx_fd,bool create=false);

        long get_file_length(RxFD fd);
    }
}



#endif // SOCKET_H
