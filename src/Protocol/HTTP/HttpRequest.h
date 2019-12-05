#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <optional>
#include "HttpDefines.h"
#include "../../Network/Connection.h"

class HttpRouter;

class HttpRequest
{
    friend class HttpRouter;
public:
    HttpRequest(RxConnection *conn):_conn_mark_closed(false),_conn_belongs(conn){}

    void clear();

    void close_connection();
    bool is_conn_mark_closed() const;

    RxChainBuffer& get_input_buf() const;
    RxChainBuffer& get_output_buf() const;

    void debug_print_header();

    HttpMethod method;
    std::string uri;
    HttpVersion version;
    HttpHeaderFields headers;

    HttpRequestBody body;

private:
    bool _conn_mark_closed;
    RxConnection *_conn_belongs;
};

#endif // HTTPREQUEST_H
