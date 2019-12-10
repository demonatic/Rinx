#include "HttpRequest.h"
#include <iostream>

void HttpReqInternal::clear()
{
    _data->_version=HttpVersion::UNKNOWN;
    _data->_method=HttpMethod::UNDEFINED;
    _data->_uri.clear();
    _data->_headers.clear();
    _data->_body.clear();
//    _conn_mark_closed=false;
}

void HttpRequest::debug_print_header()
{
    std::cout<<"----------------------"<<std::endl;
    std::cout<<"[method] "<<to_http_method_str(_data->_method)<<std::endl;
    std::cout<<"[uri] "<<_data->_uri<<std::endl;
    std::cout<<"[version] "<<to_http_version_str(_data->_version)<<std::endl;
    std::cout<<"[header fields]"<<std::endl;
    for(const auto &[field_name,field_val]:_data->_headers){
        std::cout<<"<"<<field_name<<">:<"<<field_val<<">"<<std::endl;
    }
    std::cout<<"----------------------"<<std::endl;
}

bool HttpReqInternal::is_conn_mark_closed() const
{
    return static_cast<const HttpReqImpl*>(this)->_conn_mark_closed;
}

