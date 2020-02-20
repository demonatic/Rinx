#ifndef HTTPREQHANDLERENGINE_H
#define HTTPREQHANDLERENGINE_H

#include <regex>
#include <map>
#include <mutex>
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace Rinx {

class RxProtoHttp1Processor;

class HttpRouter
{
public:
    struct Route{
        HttpMethod method;
        struct RoutableURI:public std::regex{
            RoutableURI(const char *regex_cstr):std::regex(regex_cstr,std::regex::optimize),_regex_expr(regex_cstr){}
            RoutableURI(std::string &&regex_str):std::regex(regex_str,std::regex::optimize),_regex_expr(std::move(regex_str)){}
            RoutableURI(const std::string &regex_str):std::regex(regex_str,std::regex::optimize),_regex_expr(regex_str){}
            bool operator<(const RoutableURI &other) const noexcept{ return _regex_expr<other._regex_expr; }
        private:
            std::string _regex_expr;
        }uri;
    };

public:
    void set_default_responder(const Responder default_responder);
    void set_responder_route(const Route &route,const Responder responder);

    void set_head_filter_route(const Route::RoutableURI uri,const HeadFilter filter);
    void set_body_filter_route(const Route::RoutableURI uri,const BodyFilter filter);

public:
    /// @return true if hit a responder
    bool route_to_responder(detail::HttpReqImpl &req,detail::HttpRespImpl &resp) const;

    void install_filters(detail::HttpReqImpl &req,detail::HttpRespImpl &resp) const;

    void use_default_handler(detail::HttpReqImpl &req,detail::HttpRespImpl &resp) const;

private:
    template<typename T> struct HandlerMap{
        using type=std::map<Route::RoutableURI,T>;
    };
    template<typename T>
    const static T* route(const typename HandlerMap<T>::type &map,const detail::HttpReqImpl &req);

    struct FilterHBList{
        std::list<HeadFilter> head_filter_list;
        std::list<BodyFilter> body_filter_list;
    };
    Responder _default_handler;
    std::array<HandlerMap<Responder>::type,detail::MethodCount> _responders; //use after receive a complete request
    HandlerMap<FilterHBList>::type _filters; // use on output the response
};

} //namespace Rinx

#endif // HTTPREQHANDLERENGINE_H
