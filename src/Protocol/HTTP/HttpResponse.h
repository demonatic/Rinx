#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "Network/Buffer.h"
#include "HttpDefines.h"

class HttpResponse{
public:
    using HttpResponseBody=RxChainBuffer;

    HttpResponse(RxChainBuffer &output_buff);

    void set_response_line(HttpStatusCode status,HttpVersion version){
        this->_status_code=status;
        this->_version=version;
    }

    void status_code(HttpStatusCode status){
        this->_status_code=status;
    }

    HttpStatusCode status_code() const{
        return this->_status_code;
    }

    void version(HttpVersion version){
        this->_version=version;
    }

    HttpVersion version() const{
        return this->_version;
    }

    HttpHeaderFields& headers(){
        return this->_headers;
    }

    HttpResponseBody &body();

    long body_append_istream(std::istream &ifstream,size_t stream_length);

    ///@brief flush http head and body to connection's output buffer
    void flush();

private:
    HttpStatusCode _status_code;
    HttpVersion _version;
    HttpHeaderFields _headers;

    HttpResponseBody _body;

    RxChainBuffer &_output_buff;
};


#endif // HTTPRESPONSEWRITER_H
