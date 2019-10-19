#ifndef HTTPPIPELINE_H
#define HTTPPIPELINE_H

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpReqRouter.h"
#include "Network/Connection.h"
#include <queue>


/// @brief for HTTP1.1
class HttpRequestPipeline{

    struct PipelineNode{
        PipelineNode(RxConnection *conn):request(conn),response(conn){}
        HttpRequest request;
        HttpResponse response;
        std::queue<HttpReqHandler> pending_req_handlers;
    };

public:
    HttpRequestPipeline(RxConnection *conn);
    HttpRequestPipeline(const HttpRequestPipeline &pipeline)=default;

    bool empty() const;
    PipelineNode& front();
    PipelineNode& back();

    size_t pending_request_num() const;

    void prepare_for_next_request();

    void queue_req_handler(const HttpReqHandler &req_handler);

    /// @brief try execute the first one's request handlers in its' queue,
    ///  pop it from the queue if request is complete served
    /// @return if the first one has been completed served
    bool try_handle_one();

    void try_handle_remaining();

private:
    std::queue<PipelineNode> _pipeline;
    RxConnection *_conn;
};
#endif // HTTPPIPELINE_H
