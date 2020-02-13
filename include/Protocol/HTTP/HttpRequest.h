#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <optional>
#include "HttpDefines.h"
#include "Network/Connection.h"

namespace Rinx {

struct HttpReqData{
    HttpReqData(RxConnection *conn):_conn_belongs(conn){}

    HttpMethod _method;
    std::string _uri;
    HttpVersion _version;

    HttpHeaderFields _headers;
    HttpRequestBody _body;

    RxConnection *_conn_belongs;
};

/// Internal use API
class HttpReqInternal{
public:
    HttpReqInternal(HttpReqData *data):_data(data){}

    void set_method(HttpMethod method) noexcept{
        _data->_method=method;
    }

    void set_uri(std::string uri){
        _data->_uri=std::move(uri);
    }

    void set_version(HttpVersion version) noexcept{
        _data->_version=version;
    }

    void add_header(std::string field_name,std::string field_val){
        return _data->_headers.add(std::move(field_name),std::move(field_val));
    }

    RxChainBuffer& get_input_buf() const{
        return _data->_conn_belongs->get_input_buf();
    }

    RxChainBuffer& get_output_buf() const{
        return _data->_conn_belongs->get_output_buf();
    }

    void clear();
    bool is_conn_mark_closed() const;


private:
    HttpReqData *_data;
};

/// External use API
class HttpRequest
{
    friend HttpReqInternal;
public:
    HttpRequest(HttpReqData *data):_data(data),_conn_mark_closed(false){}

    HttpMethod method() const{
        return _data->_method;
    }

    const std::string& uri() const{
        return _data->_uri;
    }

    std::string_view uri_view() const{
        return std::string_view(_data->_uri);
    }

    HttpVersion version() const{
        return _data->_version;
    }

    const HttpHeaderFields &headers() const{
        return _data->_headers;
    }

    HttpRequestBody& body() const{
        return _data->_body;
    }

    RxConnection *get_conn() const{
        return _data->_conn_belongs;
    }

    void close_connection(){
        this->_conn_mark_closed=true;
    }

    void debug_print_header();
    void debug_print_body();

private:
    HttpReqData *_data;
    bool _conn_mark_closed;
};

template<typename ...T>
struct HttpRequestTemplate:protected HttpReqData,public T...{
    HttpRequestTemplate(RxConnection *conn):HttpReqData(conn),T(this)...{}
};

using HttpReqImpl=HttpRequestTemplate<HttpReqInternal,HttpRequest>;

} //namespace Rinx
#endif // HTTPREQUEST_H
