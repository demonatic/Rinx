#include "Rinx/Protocol/HTTP/HttpReqRouter.h"

namespace Rinx {

template<typename T>
const T* HttpRouter::route(const typename HandlerMap<T>::type &map, detail::HttpReqImpl &req)
{
    const T *t=nullptr;
    for(const auto &[regex,handler]:map){
        if(std::regex_match(req.uri(),req.matches,regex)){
            t=&handler;
            break;
        }
    }
    return t;
}

bool HttpRouter::route_to_responder(detail::HttpReqImpl &req,detail::HttpRespImpl &resp) const
{
    size_t array_index=Util::to_index(req.method());
    const Responder *responder=route<Responder>(_responders[array_index],req);
    if(!responder)
        return false;

    (*responder)(req,resp);
    return true;
}

void HttpRouter::install_filters(detail::HttpReqImpl &req, detail::HttpRespImpl &resp) const
{
    const FilterHBList *filter_hb_list=route<FilterHBList>(_filters,req);
    if(!filter_hb_list)
        return;

    detail::HttpRespData::FilterChain<HeadFilter> head_filters(req,filter_hb_list->head_filter_list);
    resp.install_head_filters(head_filters);

    detail::HttpRespData::FilterChain<BodyFilter> body_filters(req,filter_hb_list->body_filter_list);
    resp.install_body_filters(body_filters);
}

void HttpRouter::set_responder_route(const Route &route,const Responder responder)
{
    if(responder){
       _responders[Util::to_index(route.method)][route.uri]=responder;
    }
}

void HttpRouter::set_head_filter_route(const Route::RoutableURI uri, const HeadFilter filter)
{
    if(filter){
        _filters[uri].head_filter_list.emplace_back(std::move(filter));
    }
}

void HttpRouter::set_body_filter_route(const HttpRouter::Route::RoutableURI uri, const BodyFilter filter)
{
    if(filter){
        _filters[uri].body_filter_list.emplace_back(std::move(filter));
    }
}

void HttpRouter::use_default_handler(detail::HttpReqImpl &req, detail::HttpRespImpl &resp) const
{
    this->_default_handler(req,resp);
}

void HttpRouter::set_default_responder(const Responder default_responder)
{
    this->_default_handler=default_responder;
}

void HttpRouter::set_www(const std::string &web_root_dir,const std::string &default_web_page)
{
    this->_www_dir=web_root_dir;
    this->_default_web_page=default_web_page;
}

const std::string &HttpRouter::get_www_dir() const
{
    return this->_www_dir;
}

const std::string &HttpRouter::get_default_web_page() const
{
    return this->_default_web_page;
}


} //namespace Rinx
