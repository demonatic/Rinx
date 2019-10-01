#include "HttpReqHandlerEngine.h"

HttpReqHandlerEngine::HttpReqHandlerEngine()
{
    set_handler({HttpMethod::GET,""},std::make_unique<StaticHttpRequestHandler>());
}

void HttpReqHandlerEngine::set_handler(HttpReqHandlerEngine::Route route, std::unique_ptr<HttpRequestHandler> handler)
{
    if(handler){
        _router_map[route.first].emplace(route.second,std::move(handler));
    }
}

int HttpReqHandlerEngine::handle_request(HttpRequest &req,HandlerAction hdlr_action)
{
//    std::cout<<"@handler engine handle request method="<<req.method<<" uri="<<req.uri<<std::endl;
    auto &uri_map=_router_map[req.method];
    for(auto &regex_pair:uri_map){
        std::smatch uri_match;
        if(std::regex_search(req.uri,uri_match,regex_pair.first)){
//            std::cout<<"@regex search ok"<<std::endl;
            HttpRequestHandler *target_handler=regex_pair.second.get();
            hdlr_action(target_handler,req);
        }
    }
    return 0;
}
