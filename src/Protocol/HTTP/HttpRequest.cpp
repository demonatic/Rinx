#include "HttpRequest.h"
#include <iostream>

void HttpRequest::clear_for_next_request()
{
    _version=HttpVersion::UNKNOWN;
    _method=HttpMethod::UNDEFINED;
    _uri.clear();
    _headers.clear();
    _body.free();
}

void HttpRequest::debug_print_header()
{
    std::cout<<"----------------------"<<std::endl;
    std::cout<<"[method] "<<to_http_method_str(_method)<<std::endl;
    std::cout<<"[uri] "<<_uri<<std::endl;
    std::cout<<"[version] "<<to_http_version_str(_version)<<std::endl;
    std::cout<<"[header fields]"<<std::endl;
    for(auto &field:_headers){
        std::cout<<"<"<<field.first<<">:<"<<field.second<<">"<<std::endl;
    }
    std::cout<<"----------------------"<<std::endl;

}
