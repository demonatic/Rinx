#include "HttpResponse.h"
#include "HttpPipeline.h"

#include <fstream>

void HttpResponse::set_async_content_provider(std::function<void ()> async_task, HttpResponse::DeferContentProvider::ProvideAction defer_content_provider)
{
    _defer_content_provider.emplace(DeferContentProvider(_conn_belongs,defer_content_provider,true));
    _conn_belongs->get_eventloop().async(async_task,[this](){
        DeferContentProvider &provider=_defer_content_provider.value();
        provider.set_status(DeferContentProvider::Status::Providing);
        std::any_cast<HttpRequestPipeline&>(_conn_belongs->data).try_handle_remaining();
    });
}

HttpResponse::HttpResponse(RxConnection *conn): _status_line(HttpStatusLine(HttpStatusCode::EMPTY,HttpVersion::VERSION_1_1),ComponentStatus::NOT_SET),
    _headers({},ComponentStatus::NOT_SET),_conn_belongs(conn)
{

}

HttpResponse::HttpResponseBody &HttpResponse::body()
{
    if(_headers.second==ComponentStatus::NOT_SET){
        throw std::runtime_error("HttpResponse: set headers before add body");
    }
    return _body;
}

void HttpResponse::flush()
{
    RxChainBuffer &output_buf=_conn_belongs->get_output_buf();

    if(this->_status_line.second==ComponentStatus::SET){
        output_buf<<to_http_version_str(this->_status_line.first.second)<<' '<<"200 OK"<<CRLF;//TODO
        this->_status_line.second=ComponentStatus::SENT;
    }
    if(this->_headers.second==ComponentStatus::SET){
        for(auto &header:this->_headers.first){
            output_buf<<header.first<<": "<<header.second<<CRLF;
        }
        output_buf<<CRLF;
        this->_headers.second=ComponentStatus::SENT;
    }
    output_buf.append(_body);
}

bool HttpResponse::DeferContentProvider::try_provide_once()
{
    RxChainBuffer &output_buf=_conn->get_output_buf();
    if(_status==Status::Providing&&output_buf.buf_slice_num()<RX_OUTPUT_BUF_CHUNK_THRESH){ //TO IMPROVE
        size_t max_length_can_write=RX_BUFFER_CHUNK_SIZE*(RX_OUTPUT_BUF_CHUNK_THRESH-output_buf.buf_slice_num());
        _provide_action(output_buf,max_length_can_write,[this](){
            _status=Status::Done;
            HttpRequest &req=std::any_cast<HttpRequestPipeline&>(_conn->data).front().request;
            if(req.stage()==HttpReqLifetimeStage::RequestReceived){
               req.stage()=HttpReqLifetimeStage::RequestCompleted;
            }
        }); //TOTEST 如果req处于wholebodyreceived状态　Done时应该设置req.lifetimestage=reqcomplete

        RxWriteRc rc;
        ssize_t n_sent=_conn->send(rc);
//                std::cout<<"@try_provide_once "<<n_sent<<std::endl;
        if(rc==RxWriteRc::ERROR){

        }
        return true;
    }
    return false;
}
