#include "Server/Server.h"
#include "Protocol/HTTP/HttpDefines.h"
#include "Protocol/HTTP/HttpRequest.h"
#include "Protocol/HTTP/HttpResponse.h"


int main(){
    RxSignalManager::disable_current_thread_signal();
    nanolog::initialize(nanolog::GuaranteedLogger(),"/tmp/","rinx_log",20);

    HttpRequestRouter::set_route({HttpMethod::GET,"/"},{{HttpReqLifetimeStage::HeaderReceived,[](HttpRequest &req,HttpResponse &resp){
        req.debug_print_header();
        resp.set_response_line(HttpStatusCode::OK,HttpVersion::VERSION_1_1);
        resp.headers().add("Content-Length","3");
        resp.body()<<"111";
    }}});
    RxServer server(65535);
    server.listen("0.0.0.0",8895);
    return 0;
}
