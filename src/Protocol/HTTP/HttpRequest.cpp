#include "HttpRequest.h"
#include <iostream>

void HttpRequest::clear()
{
    version=HttpVersion::UNKNOWN;
    method=HttpMethod::UNDEFINED;
    uri.clear();
    headers.clear();
    body.clear();
    _conn_mark_closed=false;
}

void HttpRequest::debug_print_header()
{
    std::cout<<"----------------------"<<std::endl;
    std::cout<<"[method] "<<to_http_method_str(method)<<std::endl;
    std::cout<<"[uri] "<<uri<<std::endl;
    std::cout<<"[version] "<<to_http_version_str(version)<<std::endl;
    std::cout<<"[header fields]"<<std::endl;
    for(auto field:headers){
        std::cout<<"<"<<field.first<<">:<"<<field.second<<">"<<std::endl;
    }
    std::cout<<"----------------------"<<std::endl;

}

void HttpRequest::close_connection()
{
    this->_conn_mark_closed=true;
}

bool HttpRequest::is_conn_mark_closed() const
{
    return this->_conn_mark_closed;
}

RxChainBuffer &HttpRequest::get_input_buf() const
{
    return _conn_belongs->get_input_buf();
}

RxChainBuffer &HttpRequest::get_output_buf() const
{
    return _conn_belongs->get_output_buf();
}
