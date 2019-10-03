#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <optional.hpp>
#include "HttpDefines.h"
#include "../../Network/Connection.h"

class HttpRequest
{
public:
    HttpRequest(RxConnection *conn):_conn_belongs(conn){

    }

    RxConnection *get_connection() const{
       return _conn_belongs;
    }

    std::string& uri(){
       return _uri;
    }

    HttpMethod &method(){
       return _method;
    }

    HttpVersion& version(){
       return _version;
    }

    HttpHeaderFields& headers(){
       return _headers;
    }

    void clear_for_next_request();

    void debug_print_header();

public:
    std::any req_hdlr_ctx;

private:
    HttpMethod _method;
    std::string _uri;
    HttpVersion _version;
    HttpHeaderFields _headers;

    RxChainBuffer _body; //TODO

    RxConnection *_conn_belongs;

};

#endif // HTTPREQUEST_H
