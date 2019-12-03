#ifndef HTTPREQHANDLERENGINE_H
#define HTTPREQHANDLERENGINE_H

#include "HttpDefines.h"
#include <regex>
#include <map>
#include <mutex>

class HttpRequest;
class HttpResponse;
class ContentGenerator;
class RxProtoHttp1Processor;

/// callable objects to generate response
using Responder=std::function<void(HttpRequest &req,HttpResponse &resp,ContentGenerator &generator)>;
/// callable objects to filter response
struct ChainFilter{
    using Next=std::function<void()>;
    using Filter=std::function<void(HttpRequest &req,HttpResponse &resp,Next next)>;

    ChainFilter(){}
    ChainFilter(HttpRequest &req,const std::list<Filter> &filters):_req(&req),_filters(&filters){}

    /// @brief execute the filters in sequence and return true if all filters are executed successfully
    bool operator()(HttpResponse *resp) const{
        if(!_filters)
            return true;

        auto it=_filters->cbegin();
        while(it!=_filters->cend()){
            const Filter &filter=*it;
            filter(*_req,*resp,[&](){
                ++it;
            });
        }
        return it==_filters->cend();
    }

    operator bool(){
        return _req&&_filters;
    }

private:
    HttpRequest *_req;
    const std::list<Filter> *_filters;
};

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
    /// Setters
    void set_responder_route(const Route &route,const Responder responder);
    void set_header_filter_route(const Route::RoutableURI uri,const ChainFilter::Filter filter);
    void set_body_filter_route(const Route::RoutableURI uri,const ChainFilter::Filter filter);

public:
    /// Getters
    void route_to_responder(HttpRequest &req,HttpResponse &resp) const;
    void install_filters(HttpRequest &req,HttpResponse &resp) const;

    void default_static_file_handler(HttpRequest &req,HttpResponse &resp) const;

private:
    template<typename T> struct HandlerMap{
        using type=std::map<Route::RoutableURI,T>;
    };
    template<typename T>
    const static T* route(const typename HandlerMap<T>::type &map,const HttpRequest &req);

    struct FilterHBList{
        std::list<ChainFilter::Filter> header_filter_list;
        std::list<ChainFilter::Filter> body_filter_list;
    };
    std::array<HandlerMap<Responder>::type,MethodCount> _responders; //use after receive a complete request
    HandlerMap<FilterHBList>::type _filters; // use on output the response
};

#endif // HTTPREQHANDLERENGINE_H
