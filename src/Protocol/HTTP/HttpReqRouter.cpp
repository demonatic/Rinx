#include "HttpReqRouter.h"
#include <filesystem>

template<typename T>
const T* HttpRouter::route(const typename HandlerMap<T>::type &map, const HttpReqImpl &req)
{
    const T *t=nullptr;
    for(const auto &[regex,handler]:map){
        std::smatch uri_match;
        if(std::regex_match(req.uri(),uri_match,regex)){
            t=&handler;
            break;
        }
    }
    return t;
}

void HttpRouter::route_to_responder(HttpReqImpl &req,HttpRespImpl &resp) const
{
    size_t array_index=Util::to_index(req.method());

    const Responder *responder=route<Responder>(_responders[array_index],req);
    if(!responder)
        return;

    (*responder)(req,resp);
}

void HttpRouter::install_filters(HttpReqImpl &req, HttpRespImpl &resp) const
{
    const FilterHBList *filter_hb_list=route<FilterHBList>(_filters,req);
    if(!filter_hb_list)
        return;

    HttpRespData::FilterChain<HeadFilter> head_filters(req,filter_hb_list->head_filter_list);
    resp.install_head_filters(head_filters);

    HttpRespData::FilterChain<BodyFilter> body_filters(req,filter_hb_list->body_filter_list);
    resp.install_body_filters(body_filters);
}

void HttpRouter::default_static_file_handler(HttpReqImpl &req,HttpRespImpl &resp) const
{
    static const std::filesystem::path web_root_path=WebRootPath;
    static const std::filesystem::path default_web_page=DefaultWebPage;

    std::filesystem::path authentic_path;
    try {
        //remove prefix to concat path
        std::string_view request_file(req.uri());
        while(request_file.front()=='/'){
            request_file.remove_prefix(1);
        }
        authentic_path=web_root_path/request_file;

        //check weather authentic_path is in root path
        if(std::distance(web_root_path.begin(),web_root_path.end())>std::distance(authentic_path.begin(),authentic_path.end()) //TO IMPROVE
                ||!std::equal(web_root_path.begin(),web_root_path.end(),authentic_path.begin())){
            throw std::invalid_argument("request uri must be in web root path");
        }
        if(std::filesystem::is_directory(authentic_path))
            authentic_path/=default_web_page;

        if(!std::filesystem::exists(authentic_path))
            throw std::runtime_error("file not found");

    }catch (std::exception &e) {
         LOG_INFO<<"cannot serve file: "<<req.uri()<<" exception:"<<e.what()<<" with authentic path="<<authentic_path;
         resp.send_status(HttpStatusCode::NOT_FOUND);
         return;
    }

    if(!resp.send_file_direct(req.get_conn(),authentic_path)){ //WebRootPath+'/'+DefaultWebPage
        LOG_WARN<<"send file direct failed "<<errno<<' '<<strerror(errno);
        req.close_connection();
        return;
        //TODO 是否应该让request_handler 返回 bool
    }
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
