#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "Network/Buffer.h"
#include "HttpDefines.h"

class HttpResponse{
    friend class HttpResponseWriter;
public:
    HttpResponse(RxChainBuffer &buff);

    void set_response_line(HttpStatusCode status,HttpVersion version){
        this->_status_code=status;
        this->_version=version;
    }

    void set_status_code(HttpStatusCode status){
        this->_status_code=status;
    }

    void set_version(HttpVersion version){
        this->_version=version;
    }

    void put_header(std::string key,std::string val){
        _headers.emplace_back(std::move(key),std::move(val));
    }

    void set_header(HttpHeaderFields header){
        _headers=std::move(header);
    }

    ///add http head //
    void buff_commit_head();
    ///add http body //
    long buff_commit_istream(std::istream &ifstream,size_t stream_length);

private:
    HttpStatusCode _status_code;
    HttpVersion _version;
    HttpHeaderFields _headers;

    RxChainBuffer &_output_buff;
};


#endif // HTTPRESPONSEWRITER_H
