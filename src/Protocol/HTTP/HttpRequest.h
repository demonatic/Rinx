#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include "HttpDefines.h"
#include <iostream>


struct HttpRequest
{
    HttpVersion version;
    HttpMethod method;
    std::string uri;

    HttpHeaderFields header_fields;

    void debug_print(){
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

};

#endif // HTTPREQUEST_H
