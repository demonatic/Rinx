#ifndef HTTPDEFINES_H
#define HTTPDEFINES_H

#include <vector>
#include <string_view>
#include <algorithm>
#include "../../Util/Util.h"

enum class HttpStatusCode
{
    EMPTY = 0,

    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    PROCESSING = 102,

    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTI_STATUS = 207,

    MULTIPLE_CHOISES = 300,
    MOVED_PERMANENTLY = 301,
    MOVED_TEMPORARILY = 302,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    TEMPORARY_REDIRECT = 307,

    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    methodNOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    REQUEST_ENTITY_TOO_LARGE = 413,
    REQUESTED_RANGE_NOT_SATISFIABLE = 416,

    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_versionNOT_SUPPORTED = 505,
};

enum HttpMethod{
    GET,
    POST,
    HEAD,
    PUT,
    PATCH,
    DELETE,
    ANY,
    UNDEFINED,
    HttpMethodCount
};

enum class HttpVersion{
    UNKNOWN=0,
    VERSION_1_0,
    VERSION_1_1,
    VERSION_2,
};

using HttpHeaderFields=std::vector<std::pair<std::string, std::string>>;

inline HttpMethod to_http_method(const std::string &str_method){
    HttpMethod method=HttpMethod::UNDEFINED;
    if(str_method=="GET"){
        method=HttpMethod::GET;
    }
    else if(str_method=="POST"){
        method=HttpMethod::POST;
    }
    else if(str_method=="HEAD"){
        method=HttpMethod::HEAD;
    }
    else if(str_method=="PUT"){
        method=HttpMethod::PUT;
    }
    else if(str_method=="PATCH"){
        method=HttpMethod::PATCH;
    }
    else if(str_method=="DELETE"){
        method=HttpMethod::DELETE;
    }
    return method;
}

inline std::string to_http_method_str(const HttpMethod method){
    std::string str_method;
    if(method==HttpMethod::GET){
        str_method="GET";
    }
    else if(method==HttpMethod::POST){
        str_method="POST";
    }
    else if(method==HttpMethod::HEAD){
        str_method="HEAD";
    }
    else if(method==HttpMethod::PUT){
        str_method="PUT";
    }
    else if(method==HttpMethod::PATCH){
        str_method="PATCH";
    }
    else if(method==HttpMethod::DELETE){
        str_method="DELETE";
    }
    else{
        str_method="UNDEFINED";
    }
    return str_method;
}

inline HttpVersion to_http_version(const std::string &str_version){
    HttpVersion version;
    if(str_version=="HTTP/1.0"){
        version=HttpVersion::VERSION_1_0;
    }
    else if(str_version=="HTTP/1.1"){
        version=HttpVersion::VERSION_1_1;
    }
    else if(str_version=="HTTP/2"){
        version=HttpVersion::VERSION_2;
    }
    else{
        version=HttpVersion::UNKNOWN;
    }
    return version;
}

inline std::string to_http_version_str(HttpVersion version){
    std::string str_version;
    if(version==HttpVersion::VERSION_1_0){
        str_version="HTTP/1.0";
    }
    else if(version==HttpVersion::VERSION_1_1){
        str_version="HTTP/1.1";
    }
    else if(version==HttpVersion::VERSION_2){
        str_version="HTTP/2";
    }
    else{
        str_version="UNKNOWN";
    }
    return str_version;
}

inline const char *CRLF="\r\n";

inline std::string get_mimetype_by_filename(const std::string &filename)
{
    static const std::unordered_map<std::string, std::string> mime_types{
        {"png","image/png"},
        {"html","text/html"},
        {"txt","text/plain"},
    };

    const size_t extension_pos=filename.find_last_of('.');

    std::string file_extension;
    if(extension_pos!=std::string::npos){
        file_extension=filename.substr(extension_pos+1);
        Util::to_lower(file_extension);
    }

    auto const it_mime=mime_types.find(file_extension);
    return it_mime==mime_types.end()?std::string("application/octet-stream"):it_mime->second;
}

#endif // HTTPDEFINES_H
