#ifndef LISTENER_H
#define LISTENER_H

#include <stdexcept>
#include "FD.h"

namespace Rinx {

struct RxListener{
    explicit RxListener(const uint16_t port,const std::string &address):
        port(port),bind_addr(address)
    {

    }
    void bind(){
        serv_fd=FDHelper::Stream::create_serv_sock();
        if(!FDHelper::is_open(serv_fd)||!FDHelper::Stream::set_nonblock(serv_fd,true)){
            throw std::runtime_error("fail to create listen file descriptor");
        }
        if(!FDHelper::Stream::bind(serv_fd,bind_addr.c_str(),port)){
            throw std::runtime_error("fail to bind listen file descriptor to [ address: "+bind_addr+" port: "+std::to_string(port)+']');
        }
    }
    void listen(){
        if(!FDHelper::Stream::listen(serv_fd)){
            throw std::runtime_error("fail to listen on port: "+std::to_string(port));
        }
    }
    uint16_t port;
    std::string bind_addr;
    RxFD serv_fd;
};

} //namespace Rinx

#endif // LISTENER_H
