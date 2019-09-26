#ifndef HTTPREQUESTHANDLER_H
#define HTTPREQUESTHANDLER_H

#include "HttpRequest.h"
#include "Network/Buffer.h"
#include <filesystem>
#include <fstream>

class HttpRequestHandler
{
public:
    HttpRequestHandler()=default;
    virtual ~HttpRequestHandler()=default;

    /**
     * Invoked when we have successfully fetched headers from client. This will
     * always be the first callback invoked on your handler.
     */
    virtual void on_header_fetched(HttpRequest &request)=0;

    /**
     * Invoked when we get part of body for the request.
     */
    virtual void on_part_of_body(HttpRequest &request)=0;

    /**
     * Invoked when we finish receiving the body.
     */
    virtual void on_body_finished(HttpRequest &request)=0;

};

/**
 * @brief The StaticHttpRequestHandler class is used to serve static files to client
 */
class StaticHttpRequestHandler:public HttpRequestHandler{
public:
    StaticHttpRequestHandler();
    ~StaticHttpRequestHandler() override;

    virtual void on_header_fetched(HttpRequest &request) override;
    virtual void on_part_of_body(HttpRequest &request) override;
    virtual void on_body_finished(HttpRequest &request) override;

    bool serve_file(const std::filesystem::path &uri,RxChainBuffer &buff);

private:
    static const std::filesystem::path _web_root_path;
    static const std::filesystem::path _default_page;

    std::unique_ptr<std::ifstream> _request_file;
    size_t _file_size;
    size_t _consumed_size;
};

#endif // HTTPREQUESTHANDLER_H
