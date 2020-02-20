#include "Protocol/HTTP/Http1ProtoProcessor.h"
#include <filesystem>

namespace Rinx {

RxProtoHttp1Processor::RxProtoHttp1Processor(RxConnection *conn,HttpRouter *router):
    RxProtoProcessor(conn),_router(router),_got_a_complete_req(false),_req(conn),_resp(conn),_read_timer(conn->get_eventloop())
{
    set_parser_callbacks();
    prepare_for_next_req();
}

bool RxProtoHttp1Processor::process_read_data(RxConnection *conn,RxChainBuffer &input_buf)
{
    /// Http1.1 handle request one by one
    bool err=false;
    this->extract_and_handle_request(conn,input_buf,err);
    return !err;
}

bool RxProtoHttp1Processor::handle_write_prepared(RxConnection *conn, RxChainBuffer &output_buf)
{
    bool err=false;
    this->resume(err);
    return err;
}

void RxProtoHttp1Processor::extract_and_handle_request(RxConnection *conn,RxChainBuffer &input_buf,bool &err)
{
    size_t n_left=input_buf.end()-input_buf.begin();

    while(n_left&&!err&&!_got_a_complete_req){
        HttpParser::ProcessStat proc_stat=_request_parser.parse(input_buf.begin(),input_buf.end(),&_req);
        input_buf.commit_consume(n_left-proc_stat.n_remaining);
        n_left=proc_stat.n_remaining;
        this->_got_a_complete_req=proc_stat.got_complete_request;

        if(this->try_output_response(*conn,conn->get_output_buf(),err)&&!err){
           this->prepare_for_next_req();
        }
    }
}

void RxProtoHttp1Processor::prepare_for_next_req()
{
    _req.clear();
    _resp.clear();
    _got_a_complete_req=false;
    this->set_timeout(KeepAliveTimeout);
}

void RxProtoHttp1Processor::set_parser_callbacks()
{
    _request_parser.on_event(HttpParse::StartRecvingHeader,[this](HttpReqImpl *){
        this->set_timeout(ReadHeaderTimeout);
    });

    /// callback when get http header on socket stream
    _request_parser.on_event(HttpParse::HeaderReceived,[this](HttpReqImpl *req){
        this->set_timeout(ReadBodyTimeout); //actually we don't need to care about whether body exists
        req->debug_print_header();
    });

    /// callback when get part of http body on socket stream
    _request_parser.on_event(HttpParse::OnPartofBody,[](HttpReqImpl *req,SkippedRange data){
        auto body_data=req->get_input_buf().slice(data.first,data.second);
        req->body().append(std::move(body_data));
    });

    /// callback when a complete request has received
    _request_parser.on_event(HttpParse::RequestReceived,[this](HttpReqImpl *req){
        this->cancel_read_timer();
        bool find_route=_router->route_to_responder(*req,_resp);
        if(find_route){  //try generate response to HttpResponse Object
            _router->install_filters(*req,_resp);
        }
        else{
            _router->use_default_handler(_req,_resp);
        }
    });

    /// callback when an http protocol parse error occurs
    _request_parser.on_event(HttpParse::ParseError,[this](HttpReqImpl *req){
        LOG_INFO<<"parse request error, about to closing connection..";
        _resp.send_status(HttpStatusCode::BAD_REQUEST);
        req->close_connection();
    });
}

bool RxProtoHttp1Processor::try_output_response(RxConnection &conn,RxChainBuffer &output_buf,bool &err)
{
    if(_resp.executing_async_task()){
        return false;
    }

    err=false;
    for(;;){
        if(!_resp.flush(output_buf)){
            err=true;
            break;
        }

        if(output_buf.empty()){
            break;
        }

        auto send_res=conn.send();
        if(send_res.code==RxWriteRc::SOCK_SD_BUFF_FULL){
            break;
        }
        if(send_res.code==RxWriteRc::ERROR||_req.is_conn_mark_closed()){
            LOG_INFO<<"errr mark close="<<_req.is_conn_mark_closed()<<" errno="<<strerror(errno);
            err=true;
            break;
       }
    }
    //when we recv a complete request and response has no blocking operation, we can move on to the next request,
    //despite the content yet not sent in output buf
    return _got_a_complete_req&&!_resp.has_block_operation();
}

void RxProtoHttp1Processor::resume(bool &err)
{
    //send data that already in the output buffer first
    RxConnection *conn=this->get_connection();
    RxChainBuffer &output_buf=conn->get_output_buf();

    while(!output_buf.empty()){
        RxConnection::SendRes res=conn->send();
        if(res.code==RxWriteRc::ERROR){
            LOG_WARN<<"errno when send on fd "<<conn->get_rx_fd().raw<<" errno:"<<errno<<' '<<strerror(errno);
            err=true;
            return;
        }
        if(res.code==RxWriteRc::SOCK_SD_BUFF_FULL){
            //continue to send in next writable events
            return;
        }
    }

    bool can_move_on=try_output_response(*conn,output_buf,err);
    if(err) return;

    if(can_move_on){
        prepare_for_next_req();
        extract_and_handle_request(conn,conn->get_input_buf(),err);
    }
}


void RxProtoHttp1Processor::default_static_file_handler(HttpRequest &req,HttpResponse &resp)
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
    }
}

} //namespace Rinx
