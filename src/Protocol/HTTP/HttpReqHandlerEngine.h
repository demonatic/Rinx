#ifndef HTTPREQHANDLERENGINE_H
#define HTTPREQHANDLERENGINE_H

#include "HttpRequestHandler.h"
#include "HttpRequest.h"

class HttpReqHandlerEngine
{
public:
    HttpReqHandlerEngine();

    int process_request(HttpRequest &request);
};

#endif // HTTPREQHANDLERENGINE_H
