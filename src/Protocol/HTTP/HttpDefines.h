#ifndef HTTPDEFINES_H
#define HTTPDEFINES_H

#include "../../Util/Util.h"
#include "../../Util/OnceCall.h"
#include <unordered_map>
#include <string_view>
#include <algorithm>
#include <mutex>

#define HTTP_STATUS_MAP(XX)                                                    \
  XX(100, CONTINUE, Continue)                                                  \
  XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                            \
  XX(102, PROCESSING, Processing)                                              \
  XX(200, OK, OK)                                                              \
  XX(201, CREATED, Created)                                                    \
  XX(202, ACCEPTED, Accepted)                                                  \
  XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)      \
  XX(204, NO_CONTENT, No Content)                                              \
  XX(205, RESET_CONTENT, Reset Content)                                        \
  XX(206, PARTIAL_CONTENT, Partial Content)                                    \
  XX(207, MULTI_STATUS, Multi - Status)                                        \
  XX(208, ALREADY_REPORTED, Already Reported)                                  \
  XX(226, IM_USED, IM Used)                                                    \
  XX(300, MULTIPLE_CHOICES, Multiple Choices)                                  \
  XX(301, MOVED_PERMANENTLY, Moved Permanently)                                \
  XX(302, FOUND, Found)                                                        \
  XX(303, SEE_OTHER, See Other)                                                \
  XX(304, NOT_MODIFIED, Not Modified)                                          \
  XX(305, USE_PROXY, Use Proxy)                                                \
  XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                              \
  XX(308, PERMANENT_REDIRECT, Permanent Redirect)                              \
  XX(400, BAD_REQUEST, Bad Request)                                            \
  XX(401, UNAUTHORIZED, Unauthorized)                                          \
  XX(402, PAYMENT_REQUIRED, Payment Required)                                  \
  XX(403, FORBIDDEN, Forbidden)                                                \
  XX(404, NOT_FOUND, Not Found)                                                \
  XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                              \
  XX(406, NOT_ACCEPTABLE, Not Acceptable)                                      \
  XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)        \
  XX(408, REQUEST_TIMEOUT, Request Timeout)                                    \
  XX(409, CONFLICT, Conflict)                                                  \
  XX(410, GONE, Gone)                                                          \
  XX(411, LENGTH_REQUIRED, Length Required)                                    \
  XX(412, PRECONDITION_FAILED, Precondition Failed)                            \
  XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                                \
  XX(414, URI_TOO_LONG, URI Too Long)                                          \
  XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                      \
  XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                        \
  XX(417, EXPECTATION_FAILED, Expectation Failed)                              \
  XX(421, MISDIRECTED_REQUEST, Misdirected Request)                            \
  XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                          \
  XX(423, LOCKED, Locked)                                                      \
  XX(424, FAILED_DEPENDENCY, Failed Dependency)                                \
  XX(426, UPGRADE_REQUIRED, Upgrade Required)                                  \
  XX(428, PRECONDITION_REQUIRED, Precondition Required)                        \
  XX(429, TOO_MANY_REQUESTS, Too Many Requests)                                \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large)    \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)        \
  XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                        \
  XX(501, NOT_IMPLEMENTED, Not Implemented)                                    \
  XX(502, BAD_GATEWAY, Bad Gateway)                                            \
  XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                            \
  XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                    \
  XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)              \
  XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                    \
  XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                          \
  XX(508, LOOP_DETECTED, Loop Detected)                                        \
  XX(510, NOT_EXTENDED, Not Extended)                                          \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

enum class HttpStatusCode:uint16_t
{
#define EnumDeclare(num,name,string) name=num,
    HTTP_STATUS_MAP(EnumDeclare)
#undef EnumDeclare
};

static std::string s_http_status_table[512];

static void init_http_status_table(){
#define TABLE_ENTRY_INSERT(num,name,string) s_http_status_table[num]=#num " " #string;
    HTTP_STATUS_MAP(TABLE_ENTRY_INSERT)
#undef TABLE_ENTRY_INSERT
}

static std::string to_http_status_code_str(HttpStatusCode status_code){
    static std::once_flag init_flag;
    std::call_once(init_flag,[](){
        init_http_status_table();
    });
    return s_http_status_table[Util::to_index(status_code)];
}

enum class HttpMethod:uint8_t{
    GET,
    POST,
    HEAD,
    PUT,
    PATCH,
    DELETE,
    OPTIONS,
    ANY,
    UNDEFINED,

    __HttpMethodCount
};

enum class HttpVersion:uint8_t{
    UNKNOWN=0,
    VERSION_1_0,
    VERSION_1_1,
    VERSION_2,
};

///Http request lifetime stage
enum HttpReqLifetimeStage:uint8_t{
    WaitingForHeader,
    HeaderReceived,
    RequestReceived, //the whole reqeust has received
    RequestCompleted, //reponse is fully sent

    __ReqLifetimeStageCount
};

class HttpHeaderFields{
public:
    using HeaderMap=std::unordered_multimap<std::string, std::string>;
    HttpHeaderFields()=default;

    void add(std::string field_name,std::string field_val){
        _headers.emplace(std::move(field_name),std::move(field_val));
    }

    void set_entire(std::unordered_multimap<std::string, std::string> header){
        _headers=std::move(header);
    }

    /// @brief get the corresponding header val if headers contains field_name
    /// @param id: the index of field vals of duplicate field_name
    std::optional<std::string> field_val(const std::string &field_name,const size_t id=0) const{
        auto it=_headers.find(field_name);
        std::advance(it,id);
        return it!=_headers.end()?std::make_optional<std::string>((*it).second):std::nullopt;
    }

    void remove(const std::string &key){
         _headers.erase(key);
    }

    void clear() noexcept{
        _headers.clear();
    }

    HeaderMap::iterator begin(){
        return _headers.begin();
    }

    HeaderMap::iterator end(){
        return _headers.end();
    }

private:
    HeaderMap _headers;
};

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
        {"word", "application/msword"},
         {"pdf", "application/pdf"},
         {"zip", "application/zip"},
         {"js", "application/javascript"},
         {"gif", "image/gif"},
         {"jpeg", "image/jpeg"},
         {"jpg", "image/jpeg"},
         {"png", "image/png"},
         {"css", "text/css"},
         {"html", "text/html"},
         {"htm", "text/html"},
         {"txt", "text/plain"},
         {"xml", "text/xml"},
         {"svg", "image/svg+xml"},
         {"mp4", "video/mp4"},
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
