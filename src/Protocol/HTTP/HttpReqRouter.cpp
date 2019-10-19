#include "HttpReqRouter.h"

#define HELPER_FUNC_DEFINE(Method) \
void HttpRequestRouter::Method(HttpRoute::RegexOrderable uri, HttpReqLifetimeStage stage,HttpReqHandler stage_handler)\
{\
    HttpRoute route{HttpMethod::Method,uri};\
    set_route(route,stage,stage_handler);\
}

HELPER_FUNC_DEFINE(GET)
HELPER_FUNC_DEFINE(HEAD)
HELPER_FUNC_DEFINE(POST)
HELPER_FUNC_DEFINE(PUT)
HELPER_FUNC_DEFINE(DELETE)
HELPER_FUNC_DEFINE(PATCH)
HELPER_FUNC_DEFINE(OPTIONS)

std::array<HttpRequestRouter::MethodUriMap,HttpRequestRouter::stage_count> HttpRequestRouter::_router;

bool HttpRequestRouter::route_request(HttpRequest &req,HttpResponse &resp,HttpReqLifetimeStage lifetime_stage)
{
//    std::cout<<"@handler engine handle request method="<<to_http_method_str(req.method())<<" uri="<<req.uri()<<std::endl;
    HttpReqHandler req_handler=get_route_handler(req,lifetime_stage);
    if(req_handler){
        req_handler(req,resp);
        return true;
    }
    return false;
}

void HttpRequestRouter::set_route(const HttpRoute &route, HttpReqLifetimeStage stage,HttpReqHandler stage_handler)
{
    _router[as_index(stage)][as_index(route.method)].emplace(route.uri,stage_handler);
}

HttpReqHandler HttpRequestRouter::get_route_handler(HttpRequest &req, HttpReqLifetimeStage lifetime_stage)
{
    auto &uri_map=_router[as_index(lifetime_stage)][as_index(req.method())];
    for(auto &regex_pair:uri_map){
        std::smatch uri_match;
        if(std::regex_match(req.uri(),uri_match,regex_pair.first)){
//            std::cout<<"@regex search ok"<<std::endl;
            HttpReqHandler &req_handler=regex_pair.second;
            return req_handler;
        }
    }
    return nullptr;
}
