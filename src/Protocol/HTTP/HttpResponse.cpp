#include "Protocol/HTTP/HttpResponse.h"
#include "Protocol/HTTP/ProtocolHttp1.h"
#include "Protocol/HTTP/HttpReqRouter.h"
#include <sys/sendfile.h>

namespace Rinx {

bool HttpResponse::send_file(const std::string &filename,size_t offset)
{
    try {
        FDHelper::File file(filename);

        long send_length=file.get_len()-offset;
        if(send_length<=0)
            throw std::runtime_error("error occur when send_file "+filename+" offset>=file length");

        this->status_code(HttpStatusCode::OK)
             .headers("Content-Length",std::to_string(send_length))
             .headers("Content-Type",get_mimetype_by_filename(filename));

        return this->body().read_from_regular_file(file.get_fd(),send_length,offset); //TODO changed
    }catch (std::runtime_error &e) {
        LOG_WARN<<e.what();
        return false;
    }
}

bool HttpResponse::send_file_direct(RxConnection *conn,const std::string &filename, size_t offset)
{
    try {
        FDHelper::File file(filename);

        long send_length=file.get_len()-offset;
        if(send_length<=0)
            throw std::runtime_error("error occur when send_file "+filename+" offset>=file length");

        this->status_code(HttpStatusCode::OK)
             .headers("Content-Length",std::to_string(send_length))
             .headers("Content-Type",get_mimetype_by_filename(filename));

        static_cast<HttpRespImpl*>(this)->flush(conn->get_output_buf());

        return conn->send().code==RxWriteRc::OK&&
                ::sendfile(conn->get_rx_fd().raw,file.get_fd().raw,(off64_t*)&offset,send_length)!=-1;
    }catch (std::runtime_error &e) {
        LOG_WARN<<e.what();
        return false;
    }
}

void HttpResponse::clear()
{
    _data->send_flags.status_line=Status::NOT_SET;
    _data->head.header_fields.clear();
    _data->send_flags.header_fields=Status::NOT_SET;
    _data->body.clear();
    if(_data->generator){
        _data->generator=std::make_optional<ContentGenerator>(this->_data);
    }

}

bool HttpRespInternal::flush(RxChainBuffer &output_buf)
{
    const HttpHeaderFields &headers=_data->head.header_fields;

    if(_data->send_flags.status_line==SendFlag::SET){
        const HttpStatusLine &status_line=_data->head.status_line;
        output_buf<<to_http_version_str(status_line.version)<<' '<<to_http_status_code_str(status_line.status_code)<<CRLF;
        _data->send_flags.status_line=SendFlag::SENT;

        if(!headers.contains("content-length")&&!headers.contains("Content-Length")){
            _data->send_flags.header_fields=SendFlag::SET; //in case user didn't set headers
            _data->header_filters.append(ChunkHeadFilter);
            _data->body_filters.append(ChunkBodyFilter);
        }
    }

    if(_data->send_flags.header_fields==SendFlag::SET){
        if(!(_data->header_filters)(_data->head)){ // fail to exec all filters
            return false;
        }
        for(const auto &[field_name,field_val]:headers){
            output_buf<<field_name<<": "<<field_val<<CRLF;
        }   
        this->_data->send_flags.header_fields=SendFlag::SENT;
        output_buf<<CRLF;
    }

    if(_data->generator){
        _data->generator->try_generate_once(_data->body);
    }

    if(!_data->body.empty()){
        if(!(_data->body_filters)(_data->body)){
            return false;
        }
        output_buf.append(std::move(_data->body));
    }

    if(_data->status==HttpRespData::Status::FinishWait){
        if(!(_data->body_filters)(_data->body)){
            return false;
        }
        _data->status=HttpRespData::Status::Done;
        output_buf.append(std::move(_data->body));
    }

    std::cout<<"@HttpResponse flush content:"<<std::endl;
    for(auto it=output_buf.begin();it!=output_buf.end();it++){
        std::cout<<*it;
    }
    std::cout<<std::endl;
    return true;
}

void HttpRespData::ContentGenerator::try_generate_once(HttpResponseBody &body)
{
    if(_resp_data->status==Status::Providing&&body.buf_slice_num()<OutputBufSliceThresh){
        BufAllocator allocator=[&](size_t length_expect)->uint8_t*{
            auto buf_ptr=BufferRaw::create<BufferMalloc>(length_expect);
            BufferSlice slice(buf_ptr,0,buf_ptr->length());
            body.append(slice);
            return buf_ptr->data();
        };
        ProvideDone done=[this](){
            this->set_resp_status(HttpRespData::Status::FinishWait);
        };
        _provide_action(allocator,done);
    }
}

void HttpRespData::ContentGenerator::set_content_generator(HttpRespData::ContentGenerator::ProvideAction provide_action)
{
    this->set_resp_status(Status::Providing);
    this->_provide_action=provide_action;
}

void MakeAsync::operator()(HttpRequest &req, HttpResponse &resp)
{
    RxConnection *conn=req.get_conn();
    HttpRespImpl *resp_impl=static_cast<HttpRespImpl*>(&resp);
    using Status=HttpRespData::Status;
    conn->get_eventloop()->async([=](HttpRequest &req, HttpResponse &resp) mutable{
        resp_impl->set_status(Status::ExecAsyncTask);
        _responder(req,resp);
    },
    [=]() mutable{
        Status next_stat=resp_impl->has_content_generator()?Status::Providing:Status::FinishWait;
        resp_impl->set_status(next_stat);
        RxProtoHttp1Processor &proto_proc=static_cast<RxProtoHttp1Processor&>(conn->get_proto_processor());
        bool err=false;
        proto_proc.resume(err);
        if(err){
            conn->close();
        }
    },req,resp);
}

} //namespace Rinx
