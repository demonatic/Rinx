#include "HttpResponse.h"
#include "HttpPipeline.h"

#include <fstream>

void HttpResponse::set_async_content_provider(std::function<void ()> async_task, HttpResponse::DeferContentProvider::ProvideAction defer_content_provider)
{
    _defer_content_provider.emplace(DeferContentProvider(_conn_belongs,defer_content_provider,true));
    _conn_belongs->get_eventloop()->async(async_task,[this](){
        DeferContentProvider &provider=_defer_content_provider.value();
        provider.set_status(DeferContentProvider::Status::Providing);
        std::any_cast<HttpRequestQueue&>(_conn_belongs->data).try_handle_remaining();
    });
}

bool HttpResponse::send_file(const std::string &filename,size_t offset)
{
    try {
        RxFDHelper::File file(filename);

        long send_length=file.get_len()-offset;
        if(send_length<=0)
            throw std::runtime_error("error occur when send_file "+filename+" offset>=file length");

        this->status_code(HttpStatusCode::OK)
             .headers("Content-Length",std::to_string(send_length))
             .headers("Content-Type",get_mimetype_by_filename(filename));

        return this->body().read_from_regular_file(file.get_fd(),send_length,offset);

    }catch (std::runtime_error &e) {
        LOG_WARN<<e.what();
        return false;
    }

}

HttpResponse::HttpResponse(RxConnection *conn):
    _status_line(HttpStatusLine{HttpStatusCode::OK,HttpVersion::VERSION_1_1},ComponentStatus::NOT_SET),
    _headers({},ComponentStatus::NOT_SET),_conn_belongs(conn)
{

}

void HttpResponse::flush()
{
    RxChainBuffer &output_buf=_conn_belongs->get_output_buf();

    if(this->_status_line.second==ComponentStatus::SET){
        const HttpStatusLine &status_line=this->_status_line.first;
        output_buf<<to_http_version_str(status_line.version)<<' '<<to_http_status_code_str(status_line.status_code)<<CRLF;
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

//    std::cout<<"@HttpResponse flush"<<std::endl;
//    for(auto it=output_buf.readable_begin();it!=output_buf.readable_end();it++){
//        std::cout<<*it;
//    }
//    std::cout<<std::endl;
}

bool HttpResponse::DeferContentProvider::try_provide_once()
{
    RxChainBuffer &output_buf=_conn->get_output_buf();
    if(_status==Status::Providing&&output_buf.buf_slice_num()<RX_OUTPUT_BUF_SLICE_THRESH){ //TO IMPROVE
        BufAllocator allocator=[this](size_t length_expect)->uint8_t*{
            auto buf_ptr=BufferBase::create<BufferMalloc>(length_expect);
            BufferSlice slice(buf_ptr,0,buf_ptr->length());
            HttpResponse &req=std::any_cast<HttpRequestQueue&>(_conn->data).front().response;
            req.body().push_buf_slice(slice);
            return buf_ptr->data();
        };
        ProvideDone done=[this](){
            _status=Status::Done;
            HttpRequest &req=std::any_cast<HttpRequestQueue&>(_conn->data).front().request;
            if(req.stage()==HttpReqLifetimeStage::RequestReceived){
               req.set_stage(HttpReqLifetimeStage::RequestCompleted);
            }
        };

        _provide_action(allocator,done); //TOTEST 如果req处于wholebodyreceived状态　Done时应该设置req.lifetimestage=reqcomplete

        std::cout<<"@try_provide_once "<<std::endl;

        return true;
    }
    return false;
}

HttpResponse::DeferContentProvider::Status HttpResponse::DeferContentProvider::get_status() const{
    return _status;
}
