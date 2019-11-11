#include "HttpReqRouter.h"
#include <filesystem>

#define HTTP_ROUTE_FUNC_DEFINE(Method) \
void HttpRequestRouter::Method(HttpRoute::RegexOrderable uri, HttpReqLifetimeStage stage,HttpReqHandler stage_handler)\
{\
    HttpRoute route{HttpMethod::Method,uri};\
    set_route(route,stage,stage_handler);\
}

HTTP_ROUTE_FUNC_DEFINE(GET)
HTTP_ROUTE_FUNC_DEFINE(HEAD)
HTTP_ROUTE_FUNC_DEFINE(POST)
HTTP_ROUTE_FUNC_DEFINE(PUT)
HTTP_ROUTE_FUNC_DEFINE(DELETE)
HTTP_ROUTE_FUNC_DEFINE(PATCH)
HTTP_ROUTE_FUNC_DEFINE(OPTIONS)

std::array<HttpRequestRouter::MethodUriMap,HttpRequestRouter::stage_count> HttpRequestRouter::_router;

bool HttpRequestRouter::route_request(HttpRequest &req,HttpResponse &resp,HttpReqLifetimeStage lifetime_stage)
{
    HttpReqHandler req_handler=get_route_handler(req,lifetime_stage);
    if(req_handler){
        req_handler(req,resp);
        return true;
    }
    return false;
}

void HttpRequestRouter::default_static_file_handler(HttpRequest &req,HttpResponse &resp)
{
    static const std::filesystem::path web_root_path=WebRootPath;
    static const std::filesystem::path default_web_page=DefaultWebPage;

    std::cout<<"try send file"<<std::endl;
    std::filesystem::path authentic_path;
    try {
        //remove prefix to concat path
        std::string_view request_file(req.uri());
        while(request_file.front()=='/'){
            request_file.remove_prefix(1);
        }
        authentic_path=web_root_path/request_file;

        //check weather authentic_path is in root path
        if(std::distance(web_root_path.begin(),web_root_path.end())>std::distance(authentic_path.begin(),authentic_path.end())
                ||!std::equal(web_root_path.begin(),web_root_path.end(),authentic_path.begin())){
            throw std::invalid_argument("request uri must be in web root path");
        }
        if(std::filesystem::is_directory(authentic_path))
            authentic_path/=default_web_page;

        if(!std::filesystem::exists(authentic_path))
            throw std::runtime_error("file not found");

    }catch (std::exception &e) {
         LOG_INFO<<"cannot serve file: "<<req.uri()<<" exception:"<<e.what()<<" with authentic path="<<authentic_path;
         resp.NotFound_404();
         return;
    }
    std::cout<<"authentic path="<<authentic_path<<std::endl;
    resp.send_file(authentic_path.string());
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
            HttpReqHandler &req_handler=regex_pair.second;
            return req_handler;
        }
    }
    return nullptr;
}
