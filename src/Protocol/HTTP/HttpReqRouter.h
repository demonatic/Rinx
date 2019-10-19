#ifndef HTTPREQHANDLERENGINE_H
#define HTTPREQHANDLERENGINE_H

#include <regex>
#include <map>
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

class HttpRequestRouter:RxNoncopyable
{
    #define as_index(enum_) Util::to_underlying_type(enum_)
    static constexpr auto stage_count=as_index(HttpReqLifetimeStage::ReqLifetimeStageCount);
    static constexpr auto method_count=as_index(HttpMethod::HttpMethodCount);

    #define HELPER_FUNC_DECLARE(HttpMethod) static void HttpMethod(HttpRoute::RegexOrderable uri,HttpReqLifetimeStage stage,HttpReqHandler stage_handler)

    using MethodUriMap=std::array<std::map<HttpRoute::RegexOrderable,HttpReqHandler>,method_count>;

    friend HttpRequestPipeline;
    friend RxProtoHttp1Processor;

public:
    HttpRequestRouter()=delete;
    ~HttpRequestRouter()=delete;

    HELPER_FUNC_DECLARE(GET);
    HELPER_FUNC_DECLARE(HEAD);
    HELPER_FUNC_DECLARE(POST);
    HELPER_FUNC_DECLARE(PUT);
    HELPER_FUNC_DECLARE(DELETE);
    HELPER_FUNC_DECLARE(PATCH);
    HELPER_FUNC_DECLARE(OPTIONS);

    static void set_route(const HttpRoute &route,HttpReqLifetimeStage stage,HttpReqHandler stage_handler);

private:
    static HttpReqHandler get_route_handler(HttpRequest &req,HttpReqLifetimeStage lifetime_stage);
    static bool route_request(HttpRequest &req,HttpResponse &resp,HttpReqLifetimeStage lifetime_stage);


private:
    static std::array<MethodUriMap,stage_count> _router;

};

#endif // HTTPREQHANDLERENGINE_H
