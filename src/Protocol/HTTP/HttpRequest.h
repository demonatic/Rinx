#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include "HttpDefines.h"
#include "../../Network/Connection.h"

class HttpRequest
{
public:
    void set_version(const HttpVersion version);
    HttpVersion get_version() const;

    void clear_for_next_request();
    void debug_print();

public:
    HttpVersion version;
    HttpMethod method;
    std::string uri;
    HttpHeaderFields header_fields;

    RxChainBuffer _body;

    RxConnection *conn_belongs;


};

#endif // HTTPREQUEST_H
