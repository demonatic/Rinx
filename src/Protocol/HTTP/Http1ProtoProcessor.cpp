#include "Protocol/HTTP/Http1ProtoProcessor.h"

RxProtoHttp1Processor::RxProtoHttp1Processor(RxConnection *conn,HttpRouter *router):
    RxProtoProcessor(conn),_router(router),_got_a_complete_req(false),_req(conn),_resp(conn),_read_timer(conn->get_eventloop())
{
    _request_parser.on_event(HttpParse::HeaderReceived,[this](HttpReqImpl *req){
        on_header_recv(req);
    });

    _request_parser.on_event(HttpParse::OnPartofBody,[this](HttpReqImpl *req,SkippedRange data){
        on_part_of_body(req,data);
    });

    _request_parser.on_event(HttpParse::RequestReceived,[this](HttpReqImpl *req){
        on_request_recv(req);
    });

    _request_parser.on_event(HttpParse::ParseError,[this](HttpReqImpl *req){
        on_parse_error(req);
    });
//    this->set_timeout(ReadHeaderTimeout); //not ok
}

bool RxProtoHttp1Processor::process_read_data(RxConnection *conn,RxChainBuffer &input_buf)
{
    std::cout<<"process read data fd="<<conn->get_rx_fd().raw<<std::endl;

    /// Http1.1处理完一个请求后再处理下一个
    /// 只有defer_content_provider结束后才能进行后续处理
    bool err=false;

    size_t n_left=input_buf.end()-input_buf.begin();
    std::cout<<"n_left="<<n_left<<" request_complete"<<_got_a_complete_req<<std::endl;

    while(n_left&&!err&&!_got_a_complete_req){ //当没有收到一个完整的request时进行parse
        std::cout<<"Start parsing n_left="<<n_left<<std::endl;

        HttpParser::ProcessStat proc_stat=_request_parser.parse(input_buf.begin(),input_buf.end(),&_req);
        std::cout<<"commit consume="<<n_left-proc_stat.n_remaining<<std::endl;
        input_buf.commit_consume(n_left-proc_stat.n_remaining);
        n_left=proc_stat.n_remaining;
        this->_got_a_complete_req=proc_stat.got_complete_request;

        if(this->send_respond(*conn,conn->get_output_buf(),err)&&!err){
           std::cout<<"prepare_for_next_req"<<std::endl;
           this->prepare_for_next_req();
        }
    }
    std::cout<<"process read return: "<<!err<<std::endl;

//    assert(!err==1);
    return !err;
}

bool RxProtoHttp1Processor::handle_write_prepared(RxConnection *conn, RxChainBuffer &output_buf)
{
    //send data that already in the output buffer first
    std::cout<<"@handle_write_prepared"<<std::endl;
    while(!output_buf.empty()){
        RxConnection::SendRes res=conn->send();
        std::cout<<"write_prepar send="<<res.send_len<<std::endl;
        if(res.code==RxWriteRc::ERROR){
            LOG_WARN<<"errno when send on fd "<<conn->get_rx_fd().raw<<" errno:"<<errno<<' '<<strerror(errno);
            return false;
        }
        if(res.code==RxWriteRc::SOCK_SD_BUFF_FULL){
            //continue to send in next writable events
            return true;
        }
    }

    bool err;
    this->send_respond(*conn,output_buf,err);
    return err;
}

void RxProtoHttp1Processor::on_header_recv(HttpReqImpl *http_request)
{
//    std::cout<<"HeaderReceived"<<std::endl;
    http_request->debug_print_header();

//    this->set_timeout(ReadBodyTimeout); //TODO has body?

}

void RxProtoHttp1Processor::on_part_of_body(HttpReqImpl *http_request,SkippedRange data)
{

    auto body_data=http_request->get_input_buf().slice(data.first,data.second);

    http_request->body().append(std::move(body_data));
    http_request->debug_print_body();
}

void RxProtoHttp1Processor::on_request_recv(HttpReqImpl *http_request)
{
//    std::cout<<"RequestReceived uri="<<http_request->uri()<<std::endl;

    _router->route_to_responder(*http_request,_resp);

    if(_resp.empty()){
        _router->default_static_file_handler(_req,_resp);
    }
    else{
        _router->install_filters(*http_request,_resp);
    }
    this->set_timeout(KeepAliveTimeout);
}

void RxProtoHttp1Processor::on_parse_error(HttpReqImpl *http_request)
{
    LOG_INFO<<"parse request error, closing connection..";
    _resp.send_status(HttpStatusCode::BAD_REQUEST);
    http_request->close_connection();
}

void RxProtoHttp1Processor::prepare_for_next_req()
{
    _req.clear();
    _resp.clear();
    _got_a_complete_req=false;
    req_count++;
    std::cout<<"req count="<<req_count<<std::endl;
}

bool RxProtoHttp1Processor::send_respond(RxConnection &conn,RxChainBuffer &output_buf,bool &err)
{
    err=false;
    for(;;){
        if(!_resp.flush(output_buf)){
            err=true;
            break;
        }

        if(output_buf.empty()){
            std::cout<<"empty output buf"<<std::endl;
            break;
        }

        auto send_res=conn.send();
        std::cout<<"n_send="<<send_res.send_len<<std::endl;
        if(send_res.code==RxWriteRc::SOCK_SD_BUFF_FULL){
            std::cout<<"buff full"<<std::endl;
            break;
        }

        if(send_res.code==RxWriteRc::ERROR||_req.is_conn_mark_closed()){
            std::cout<<"errr mark close="<<_req.is_conn_mark_closed()<<" errno="<<strerror(errno)<<std::endl;
            err=true;
            break;
       }
    }
    std::cout<<"has blocking oepration "<<_resp.has_block_operation()<<std::endl;
    return _got_a_complete_req&&!_resp.has_block_operation();
}

