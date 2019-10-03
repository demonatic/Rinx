#include "HttpReqRouter.h"

std::array<HttpRequestRouter::MethodUriMap,HttpRequestRouter::stage_count> HttpRequestRouter::_router;

bool HttpRequestRouter::route_request(HttpRequest &req,HttpReqLifetimeStage lifetime_stage)
{
//    std::cout<<"@handler engine handle request method="<<to_http_method_str(req.method())<<" uri="<<req.uri()<<std::endl;
    auto &uri_map=_router[as_index(lifetime_stage)][as_index(req.method())];
    for(auto &regex_pair:uri_map){
        std::smatch uri_match;
        if(std::regex_match(req.uri(),uri_match,regex_pair.first)){
//            std::cout<<"@regex search ok"<<std::endl;
            HttpReqHandler &req_handler=regex_pair.second;
            HttpResponse resp(req.get_connection()->get_output_buf());
            req_handler(req,resp);
            resp.flush();
            return true;
        }
    }

    return false;
}

void HttpRequestRouter::set_route(const HttpRoute &route, const std::unordered_map<HttpReqLifetimeStage,HttpReqHandler> &stage_handlers)
{
    for(auto &stage_handler:stage_handlers){
        const HttpReqLifetimeStage &stage=stage_handler.first;
        const HttpReqHandler &req_handler=stage_handler.second;
        _router[as_index(stage)][as_index(route.method)].emplace(route.uri,req_handler);
    }
}
