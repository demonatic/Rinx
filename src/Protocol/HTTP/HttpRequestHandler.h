#ifndef HTTPREQUESTHANDLER_H
#define HTTPREQUESTHANDLER_H

#include "HttpRequest.h"

class HttpRequestHandler
{
public:
    HttpRequestHandler();
    /**
     * Invoked when we have successfully fetched headers from client. This will
     * always be the first callback invoked on your handler.
     */
    void on_request(HttpRequest &request);


    /**
     * Invoked when we get part of body for the request.
     */
    void on_body(HttpRequest &request);
};

#endif // HTTPREQUESTHANDLER_H
