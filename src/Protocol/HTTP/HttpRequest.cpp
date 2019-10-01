#include "HttpRequest.h"
#include <iostream>

void HttpRequest::clear_for_next_request()
{
    version=HttpVersion::UNKNOWN;
    method=HttpMethod::UNDEFINED;
    uri.clear();
    header_fields.clear();
}

void HttpRequest::debug_print()
{
    std::cout<<"----------------------"<<std::endl;
    std::cout<<"[method] "<<to_http_method_str(method)<<std::endl;
    std::cout<<"[uri] "<<uri<<std::endl;
    std::cout<<"[version] "<<to_http_version_str(version)<<std::endl;
    std::cout<<"[header fields]"<<std::endl;
    for(auto field:header_fields){
        std::cout<<"<"<<field.first<<">:<"<<field.second<<">"<<std::endl;
    }
    std::cout<<"----------------------"<<std::endl;

}
