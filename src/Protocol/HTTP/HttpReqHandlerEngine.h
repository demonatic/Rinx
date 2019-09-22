#ifndef HTTPREQHANDLERENGINE_H
#define HTTPREQHANDLERENGINE_H

#include <regex>
#include <map>
#include "HttpRequestHandler.h"
#include "HttpRequest.h"


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

class HttpReqHandlerEngine
{
    using Route=std::pair<HttpMethod,RegexOrderable>;
    using HandlerAction=std::function<void(HttpRequestHandler *hdr,HttpRequest &req)>;
public:
    HttpReqHandlerEngine();
    void set_handler(Route route,std::unique_ptr<HttpRequestHandler> handler);

    int handle_request(HttpRequest &req, HandlerAction hdlr_action);

private:
    std::array<std::map<RegexOrderable,std::unique_ptr<HttpRequestHandler>>,HttpMethodCount> _router_map;

};

#endif // HTTPREQHANDLERENGINE_H
