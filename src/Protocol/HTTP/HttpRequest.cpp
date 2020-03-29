#include "Rinx/Protocol/HTTP/HttpRequest.h"
#include <iostream>

namespace Rinx {

std::string HttpRequest::decode_uri() const
{
    std::string decoded;
    detail::uri_decode_impl(_data->_uri.begin(),_data->_uri.end(),std::back_inserter(decoded));
    return decoded;
}

void HttpRequest::debug_print_header()
{
    std::cout<<"----------------------"<<std::endl;
    std::cout<<"[method] "<<detail::to_http_method_str(_data->_method)<<std::endl;
    std::cout<<"[uri] "<<_data->_uri<<std::endl;
    std::cout<<"[version] "<<detail::to_http_version_str(_data->_version)<<std::endl;
    std::cout<<"[header fields]"<<std::endl;
    for(const auto &[field_name,field_val]:_data->_headers){
        std::cout<<"<"<<field_name<<">:<"<<field_val<<">"<<std::endl;
    }
    std::cout<<"----------------------"<<std::endl;
}

void HttpRequest::debug_print_body()
{
    std::cout<<"-----------body start--------------"<<std::endl;
    for(auto it=_data->_body.begin();it!=_data->_body.end();++it){
        std::cout<<*it;
    }
    std::cout<<std::endl;
    std::cout<<"-----------body end--------------"<<std::endl;
}

void detail::HttpReqInternal::clear()
{
    _data->_version=HttpVersion::UNKNOWN;
    _data->_method=HttpMethod::UNDEFINED;
    _data->_uri.clear();
    _data->_headers.clear();
    _data->_body.clear();
}

bool detail::HttpReqInternal::is_conn_mark_closed() const
{
    return _data->_conn_mark_closed;
}

} //namespace Rinx
