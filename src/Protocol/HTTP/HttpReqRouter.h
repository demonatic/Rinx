#ifndef HTTPREQHANDLERENGINE_H
#define HTTPREQHANDLERENGINE_H

#include <regex>
#include <map>
#include <mutex>
#include "HttpRequest.h"
#include "HttpResponse.h"

class HttpRequestPipeline;
class RxProtoHttp1Processor;

using HttpReqHandler=std::function<void(HttpRequest&,HttpResponse&)>;

struct HttpRoute{
    class RegexOrderable:public std::regex{
    public:
        RegexOrderable(const char *regex_cstr):std::regex(regex_cstr,std::regex::optimize),_regex_expr(regex_cstr){}
        RegexOrderable(std::string &&regex_str):std::regex(regex_str,std::regex::optimize),_regex_expr(std::move(regex_str)){}

        bool operator<(const RegexOrderable &other) const noexcept{
            return _regex_expr<other._regex_expr;
        }
    private:
        std::string _regex_expr;
    };

    HttpMethod method;
    RegexOrderable uri;
};

class HttpRequestRouter
{
    friend HttpRequestPipeline;
    friend RxProtoHttp1Processor;
    #define as_index(_enum) Util::to_index(_enum)
    static constexpr auto stage_count=as_index(HttpReqLifetimeStage::__ReqLifetimeStageCount);
    static constexpr auto method_count=as_index(HttpMethod::__HttpMethodCount);

    using MethodUriMap=std::array<std::map<HttpRoute::RegexOrderable,HttpReqHandler>,method_count>;
public:
    HttpRequestRouter()=delete;
    HttpRequestRouter(const HttpRequestRouter&)=delete;

    #define HTTP_ROUTE_FUNC_DECLARE(HttpMethod)\
    static void HttpMethod(HttpRoute::RegexOrderable uri,HttpReqLifetimeStage stage,HttpReqHandler stage_handler)

    HTTP_ROUTE_FUNC_DECLARE(GET);
    HTTP_ROUTE_FUNC_DECLARE(HEAD);
    HTTP_ROUTE_FUNC_DECLARE(POST);
    HTTP_ROUTE_FUNC_DECLARE(PUT);
    HTTP_ROUTE_FUNC_DECLARE(DELETE);
    HTTP_ROUTE_FUNC_DECLARE(PATCH);
    HTTP_ROUTE_FUNC_DECLARE(OPTIONS);

private:
    static void set_route(const HttpRoute &route,HttpReqLifetimeStage stage,HttpReqHandler stage_handler);

    static HttpReqHandler get_route_handler(HttpRequest &req,HttpReqLifetimeStage lifetime_stage);

    static bool route_request(HttpRequest &req,HttpResponse &resp,HttpReqLifetimeStage lifetime_stage);

    static void default_static_file_handler(HttpRequest &req,HttpResponse &resp);

private:
    static std::array<MethodUriMap,stage_count> _router;
};

#endif // HTTPREQHANDLERENGINE_H
