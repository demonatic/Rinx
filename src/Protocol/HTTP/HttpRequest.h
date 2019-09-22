#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include "HttpDefines.h"
#include "../../Network/Connection.h"

struct HttpRequest
{
    HttpVersion version;
    HttpMethod method;
    std::string uri;

    HttpHeaderFields header_fields;
    RxConnection *conn_belongs;

    void reset();
    void debug_print();

};

#endif // HTTPREQUEST_H
