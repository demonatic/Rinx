#include "Server/Server.h"
#include "Protocol/HTTP/HttpDefines.h"
#include "Protocol/HTTP/HttpRequest.h"
#include "Protocol/HTTP/HttpResponse.h"


int main(){
    RxSignalManager::disable_current_thread_signal();
    nanolog::initialize(nanolog::GuaranteedLogger(),"/tmp/","rinx_log",20);

    //TODO 处理用户在不同stage写resp字段混乱的问题
    /// 用户必须在某个stage一次性写完header,body可以分多次写

    HttpRequestRouter::GET("/",HttpReqLifetimeStage::HeaderReceived,[](HttpRequest &req,HttpResponse &resp){
        req.debug_print_header();

        resp.status_code(HttpStatusCode::OK)
            .headers("Content-Length","23")
            .body()<<"response data";

        resp.set_content_provider([](RxChainBuffer &body,size_t max_length_expected,HttpResponse::ProvideDone done){
             body<<"large data";
             done();
        });

    });
    HttpRequestRouter::GET("/",HttpReqLifetimeStage::RequestCompleted,[](HttpRequest &req,HttpResponse &resp){
        std::cout<<"@handler /: request compleleted"<<std::endl;
    });

    RxServer server;
    server.listen("0.0.0.0",7788);
    return 0;
}
