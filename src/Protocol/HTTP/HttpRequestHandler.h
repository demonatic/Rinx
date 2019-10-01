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

protected:
    std::function<void(HttpRequest& request)> _on_resume_write;
};

/**
 * @brief The StaticHttpRequestHandler class is used to serve static files to client
 */
class StaticHttpRequestHandler:public HttpRequestHandler{
public:
    StaticHttpRequestHandler();
    virtual ~StaticHttpRequestHandler() override;

    virtual void on_header_fetched(HttpRequest &request) override;
    virtual void on_part_of_body(HttpRequest &request) override;
    virtual void on_body_finished(HttpRequest &request) override;

    bool start_serve_file(const std::filesystem::path &uri,RxChainBuffer &buff);
    bool resume_serve_file(RxChainBuffer &buff);

    bool is_file_send_done();
    void reset();

private:
    static const std::filesystem::path _web_root_path;
    static const std::filesystem::path _default_page;
    static const size_t one_round_length_limit=16384;

    std::unique_ptr<std::ifstream> _request_file;
    size_t _file_size;
    size_t _consumed_size;
};

#endif // HTTPREQUESTHANDLER_H
