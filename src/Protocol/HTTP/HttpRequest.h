#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include "HttpDefines.h"

struct HttpRequest
{
    HttpVersion version;
    HttpMethod method;
    std::string uri;

};

#endif // HTTPREQUEST_H
