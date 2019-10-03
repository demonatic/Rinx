#ifndef HTTPREQHANDLERENGINE_H
#define HTTPREQHANDLERENGINE_H

#include <regex>
#include <map>
#include "HttpRequest.h"
#include "HttpResponse.h"

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
    using HttpReqHandler=std::function<void(HttpRequest&,HttpResponse&)>;
    using MethodUriMap=std::array<std::map<HttpRoute::RegexOrderable,HttpReqHandler>,method_count>;

public:
    HttpRequestRouter()=delete ;
    ~HttpRequestRouter()=delete;

    static void test(int i);
    static void set_route(const HttpRoute &route,const std::unordered_map<HttpReqLifetimeStage,HttpReqHandler> &stage_handlers);

    static bool route_request(HttpRequest &req,HttpReqLifetimeStage lifetime_stage);

private:
    static std::array<MethodUriMap,stage_count> _router;

};

#endif // HTTPREQHANDLERENGINE_H
