#include "Protocol/HTTP/Http1ProtoProcessor.h"

namespace Rinx {

RxProtoHttp1Processor::RxProtoHttp1Processor(RxConnection *conn,HttpRouter *router):
    RxProtoProcessor(conn),_router(router),_got_a_complete_req(false),_req(conn),_resp(conn),_read_timer(conn->get_eventloop())
{
    set_parser_callbacks();
    prepare_for_next_req();
    std::cout<<"@RxProtoHttp1Processor tid="<<std::this_thread::get_id()<<std::endl;
}

bool RxProtoHttp1Processor::process_read_data(RxConnection *conn,RxChainBuffer &input_buf)
{
    std::cout<<"process read data fd="<<conn->get_rx_fd().raw<<std::endl;

    /// Http1.1处理完一个请求后再处理下一个
    /// 只有defer_content_provider结束后才能进行后续处理
    bool err=false;
    this->extract_and_handle_request(conn,input_buf,err);
    return !err;
}

bool RxProtoHttp1Processor::handle_write_prepared(RxConnection *conn, RxChainBuffer &output_buf)
{
    std::cout<<"@handle_write_prepared"<<std::endl;
    bool err=false;
    this->resume(err);
    return err;
}

void RxProtoHttp1Processor::extract_and_handle_request(RxConnection *conn,RxChainBuffer &input_buf,bool &err)
{
    size_t n_left=input_buf.end()-input_buf.begin();
    std::cout<<"n_left="<<n_left<<" request_complete"<<_got_a_complete_req<<std::endl;

    while(n_left&&!err&&!_got_a_complete_req){ //当没有收到一个完整的request时进行parse
        std::cout<<"Start parsing n_left="<<n_left<<std::endl;

        HttpParser::ProcessStat proc_stat=_request_parser.parse(input_buf.begin(),input_buf.end(),&_req);
        std::cout<<"commit consume="<<n_left-proc_stat.n_remaining<<std::endl;
        input_buf.commit_consume(n_left-proc_stat.n_remaining);
        n_left=proc_stat.n_remaining;
        this->_got_a_complete_req=proc_stat.got_complete_request;

        if(this->try_output_response(*conn,conn->get_output_buf(),err)&&!err){
           std::cout<<"prepare_for_next_req"<<std::endl;
           this->prepare_for_next_req();
        }
    }
    std::cout<<"process read return: "<<!err<<std::endl;
//    assert(!err==1);
}

void RxProtoHttp1Processor::prepare_for_next_req()
{
    _req.clear();
    _resp.clear();
    _got_a_complete_req=false;
    this->set_timeout(KeepAliveTimeout);
    req_count++;
    std::cout<<"req count="<<req_count<<std::endl;
}

void RxProtoHttp1Processor::set_parser_callbacks()
{
    _request_parser.on_event(HttpParse::StartRecvingHeader,[this](HttpReqImpl *){
        this->set_timeout(ReadHeaderTimeout);
    });

    /// callback when get http header on socket stream
    _request_parser.on_event(HttpParse::HeaderReceived,[this](HttpReqImpl *req){
        //    std::cout<<"HeaderReceived"<<std::endl;
        this->set_timeout(ReadBodyTimeout);
        req->debug_print_header();
    });

    /// callback when get part of http body on socket stream
    _request_parser.on_event(HttpParse::OnPartofBody,[](HttpReqImpl *req,SkippedRange data){
        auto body_data=req->get_input_buf().slice(data.first,data.second);
        req->body().append(std::move(body_data));
        req->debug_print_body();
    });

    /// callback when a complete request has received
    _request_parser.on_event(HttpParse::RequestReceived,[this](HttpReqImpl *req){
        this->cancel_read_timer();
        //    std::cout<<"RequestReceived uri="<<http_request->uri()<<std::endl;
        if(_router->route_to_responder(*req,_resp)){  //try generate response to HttpResponse Object
            _router->install_filters(*req,_resp);
        }
        else{
            _router->default_static_file_handler(_req,_resp);
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

    static size_t send_len=0;
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

        send_len+=send_res.send_len;
        std::cout<<"round send="<<send_res.send_len<<std::endl;
        std::cout<<"send length="<<send_len<<std::endl;

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
        std::cout<<"write_prepar send="<<res.send_len<<std::endl;
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

} //namespace Rinx
