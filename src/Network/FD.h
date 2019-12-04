#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/eventfd.h>
#include <vector>
#include <string>

enum RxFDType{
    FD_LISTEN=0,
    FD_CLIENT_STREAM,
    FD_EPOLL,
    FD_EVENT,
    FD_REGULAR_FILE,
    FD_INVALID,

    __RxFD_TYPE_COUNT,
};

struct RxFD{
    int raw;
    RxFDType type;

    constexpr RxFD(RxFDType type,int fd):raw(fd),type(type){}
    constexpr RxFD():raw(-1),type(RxFDType::FD_INVALID){}

    constexpr operator int() const{return raw;}
    constexpr bool operator==(const RxFD &other) const {return this->type==other.type&&this->raw==other.raw;}
    constexpr bool operator!=(const RxFD &other) const {return !((*this)==other);}
};

constexpr static RxFD RxInvalidFD={RxFDType::FD_INVALID,-1};

enum class RxAcceptRc:uint8_t{
    ACCEPTING,
    ALL_ACCEPTED,
    FAILED,
    ERROR
};

enum class RxReadRc:uint8_t{
    OK,
    CLOSED,
    ERROR,
};

enum class RxWriteRc:uint8_t{
    OK,
    SYS_SOCK_BUFF_FULL,
    ERROR
};

// TODO change int fd to RxFD
namespace RxFDHelper{

    /// @brief close the fd and reset fd to InvalidFD
    /// @return whether the close operation is executed successfully
    bool close(RxFD &fd) noexcept;

    /// @brief check whether the fd is -1
    bool is_open(RxFD fd) noexcept;

    namespace Stream{
        RxFD create_client_stream() noexcept;
        RxFD create_serv_sock() noexcept;

        bool shutdown_both(RxFD fd) noexcept;

        bool bind(RxFD fd,const char *host,const int port) noexcept;
        bool listen(RxFD fd) noexcept;
        RxFD accept(RxFD fd,RxAcceptRc &accept_res) noexcept;

        bool set_tcp_nodelay(RxFD fd,const bool nodelay) noexcept;
        bool set_nonblock(RxFD fd,const bool nonblock) noexcept;

        ssize_t read(RxFD fd,void *buffer,size_t n,RxReadRc &read_res);
        ssize_t readv(RxFD fd,std::vector<struct iovec> &io_vec,RxReadRc &read_res);

        ssize_t write(RxFD fd,void *buffer,size_t n,RxWriteRc &write_res);
        ssize_t writev(RxFD fd,std::vector<struct iovec> &io_vec,RxWriteRc &write_res);

    };

    namespace Event{
        RxFD create_event_fd() noexcept;

        bool read_event_fd(RxFD fd);
        bool write_event_fd(RxFD fd);
    }

    namespace RegFile{
        /// @return if the file is opened successfully
        bool open(const std::string &path,RxFD &rx_fd,bool create=false);

        long get_file_length(RxFD fd);
    }

    class File{
    public:
        File(const std::string &path,bool create=false);
        ~File();

        long get_len() const;
        RxFD get_fd() const;

    private:
        RxFD _file_fd;
    };

}



#endif // SOCKET_H
