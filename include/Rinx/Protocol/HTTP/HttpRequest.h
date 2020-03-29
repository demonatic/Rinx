#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <optional>
#include <regex>
#include "Rinx/Protocol/HTTP/HttpDefines.h"
#include "Rinx/Network/Connection.h"

namespace Rinx {

namespace detail{

struct HttpReqData{
    HttpReqData(RxConnection *conn):_conn_belongs(conn){}

    HttpMethod _method;
    std::string _uri;
    HttpVersion _version;

    HttpHeaderFields _headers;
    HttpRequestBody _body;

    bool _conn_mark_closed=false;
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

}

/// External use API
class HttpRequest
{
    friend detail::HttpReqInternal;
public:
    HttpRequest(detail::HttpReqData *data):_data(data){}

    HttpMethod method() const{
        return _data->_method;
    }

    const std::string& uri() const{
        return _data->_uri;
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
        this->_data->_conn_mark_closed=true;
    }

    std::string decode_uri() const;

    void debug_print_header();
    void debug_print_body();

public:
    std::smatch matches;
private:
    detail::HttpReqData *_data;
};

namespace detail {

template<typename ...T>
struct HttpRequestTemplate:protected HttpReqData,public T...{
    HttpRequestTemplate(RxConnection *conn):HttpReqData(conn),T(this)...{}
};

using HttpReqImpl=HttpRequestTemplate<HttpReqInternal,HttpRequest>;


/// uri-decode from https://cpp-netlib.org/ boost::network::uri
template<typename Ch>
Ch letter_to_hex(Ch c){
    switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return c - '0';
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        return c + 10 - 'a';
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        return c + 10 - 'A';
    }
    return Ch();
}

template<class InputIterator,class OutputIterator>
OutputIterator uri_decode_impl(const InputIterator begin,const InputIterator end,OutputIterator out){
    using value_type=typename std::iterator_traits<InputIterator>::value_type;
    InputIterator it=begin;
    while(it!=end){
        if(*it=='%'){
            if(++it==end) throw std::out_of_range("unexpected end of stream");
            value_type v1=letter_to_hex(*it);
            if(++it==end) throw std::out_of_range("unexpected end of stream");
            value_type v2=letter_to_hex(*it);
            ++it;
            *out++=0x10*v1+v2;
        }
        else if(*it=='+'){
            *out++=' ';
            ++it;
        }
        else{
            *out++=*it++;
        }
    }
    return out;
}
}

} //namespace Rinx
#endif // HTTPREQUEST_H
