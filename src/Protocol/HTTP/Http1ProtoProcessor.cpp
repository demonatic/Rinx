#include "Http1ProtoProcessor.h"

RxProtoHttp1Processor::RxProtoHttp1Processor(RxConnection *conn,HttpRouter *router):
    RxProtoProcessor(conn),_router(router),_got_a_complete_req(false),_req(conn),_resp(conn),_read_timer(conn->get_eventloop())
{
#define REGISTER_PARSER_EVENT_CB(ParserEvent,callback)\
    _request_parser.on_event(HttpParse::ParserEvent,[this](InputDataRange src,HttpRequest *req){\
        callback(src,req); \
    })

    REGISTER_PARSER_EVENT_CB(HeaderReceived,on_header_recv);
    REGISTER_PARSER_EVENT_CB(OnPartofBody,on_part_of_body);
    REGISTER_PARSER_EVENT_CB(RequestReceived,on_request_recv);
    REGISTER_PARSER_EVENT_CB(ParseError,on_parse_error);

#undef REGISTER_PARSER_EVENT_CB


//    this->set_timeout(ReadHeaderTimeout); //not ok
}

bool RxProtoHttp1Processor::process_read_data(RxConnection *conn,RxChainBuffer &input_buf)
{
    std::cout<<"process read data fd="<<conn->get_rx_fd().raw<<std::endl;

    /// Http1.1处理完一个请求后再处理下一个
    /// 只有defer_content_provider结束后才能进行后续处理
    bool err=false;

    size_t n_left=input_buf.readable_end()-input_buf.readable_begin();
    std::cout<<"n_left="<<n_left<<" request_complete"<<_got_a_complete_req<<std::endl;
    while(n_left&&!err&&!_got_a_complete_req){ //当没有收到一个完整的request时进行parse
        std::cout<<"Start parsing"<<std::endl;
        HttpParser::ProcessStat proc_stat=_request_parser.parse(input_buf.readable_begin(),input_buf.readable_end(),&_req);

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
    while(!output_buf.empty()){
        RxConnection::SendRes res=conn->send();
        if(res.code==RxWriteRc::ERROR){
            LOG_WARN<<"errno when send on fd "<<conn->get_rx_fd().raw<<" errno:"<<errno<<' '<<strerror(errno);
            return false;
        }
        if(res.code==RxWriteRc::SYS_SOCK_BUFF_FULL){
            //continue to send in next writable events
            return true;
        }
    }

    bool err;
    this->send_respond(*conn,output_buf,err);
    return err;
}

void RxProtoHttp1Processor::on_header_recv(InputDataRange src, HttpRequest *http_request)
{
    std::cout<<"HeaderReceived"<<std::endl;

//    this->set_timeout(ReadBodyTimeout); //TODO has body?


}

void RxProtoHttp1Processor::on_part_of_body(InputDataRange src, HttpRequest *http_request)
{
    http_request->body.append(http_request->get_input_buf().slice(src.first,src.second));
}

void RxProtoHttp1Processor::on_request_recv(InputDataRange src, HttpRequest *http_request)
{
    std::cout<<"RequestReceived uri="<<http_request->uri<<std::endl;

    _router->route_to_responder(*http_request,_resp);

    if(_resp.empty()){
        _router->default_static_file_handler(_req,_resp);
    }
    else{
        _router->install_filters(*http_request,_resp);
    }
    this->set_timeout(KeepAliveTimeout);
}

void RxProtoHttp1Processor::on_parse_error(InputDataRange src, HttpRequest *http_request)
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
}

bool RxProtoHttp1Processor::send_respond(RxConnection &conn,RxChainBuffer &output_buf,bool &err)
{
    err=false;
    for(;;){
        _resp.flush(output_buf);

        if(output_buf.empty()){
            std::cout<<"empty output buf"<<std::endl;
            break;
        }

        auto send_res=conn.send();
        std::cout<<"n_send="<<send_res.send_len<<std::endl;
        if(send_res.code==RxWriteRc::SYS_SOCK_BUFF_FULL){
            std::cout<<"buff full"<<std::endl;
            break;
        }

        if(send_res.code==RxWriteRc::ERROR||_req.is_conn_mark_closed()){
            std::cout<<"errr"<<std::endl;
            err=true;
            return true;
       }
    }
    return _got_a_complete_req&&!_resp.has_block_operation();
}

