#ifndef LISTENER_H
#define LISTENER_H

#include <stdexcept>
#include "FD.h"

namespace Rinx {

struct RxListener{
    explicit RxListener(uint16_t port):port(port),serv_fd(FDHelper::Stream::create_serv_sock()){
        if(!FDHelper::is_open(serv_fd)||!FDHelper::Stream::set_nonblock(serv_fd,true)){
            throw std::runtime_error("fail to create listen file descriptor");
        }
    }
    void bind(const std::string &address,uint16_t port){
        if(!FDHelper::Stream::bind(serv_fd,address.c_str(),port)){
            throw std::runtime_error("fail to bind listen file descriptor to [ address: "+address+" port: "+std::to_string(port)+']');
        }
    }
    void listen(){
        if(!FDHelper::Stream::listen(serv_fd)){
            throw std::runtime_error("fail to listen on port: "+std::to_string(port));
        }
    }
    uint16_t port;
    RxFD serv_fd;
};

} //namespace Rinx

#endif // LISTENER_H
