#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "Network/Buffer.h"
#include "HttpDefines.h"

class HttpResponse{
    friend class HttpResponseWriter;
public:
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

private:
    HttpStatusCode _status_code;
    HttpVersion _version;
    HttpHeaderFields _headers;
};

class HttpResponseWriter
{
public:
    HttpResponseWriter(RxChainBuffer &buff);
    void write_http_head(HttpResponse &response);
    long write_fstream(std::ifstream &ifstream,size_t stream_length);

private:
    RxChainBuffer &_buff;
};

#endif // HTTPRESPONSEWRITER_H
