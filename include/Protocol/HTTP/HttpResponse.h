#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "Network/Buffer.h"
#include "Network/Connection.h"
#include "HttpDefines.h"
#include "HttpRequest.h"
#include <atomic>

namespace Rinx {

class HttpResponse;
class RxProtoHttp1Processor;

/// callable objects to generate response
using Responder=std::function<void(HttpRequest &req,HttpResponse &resp)>;

/// callable objects to filter response
using Next=std::function<void()>;
using HeadFilter=std::function<void(HttpRequest &req,HttpResponseHead &head,Next next)>;
using BodyFilter=std::function<void(HttpRequest &req,HttpResponseBody &body,Next next)>;

namespace detail{

struct HttpRespData{
    HttpRespData(RxConnection *conn):head{{HttpStatusCode::OK,HttpVersion::VERSION_1_1},{}},
        generator(std::nullopt),status(Status::FinishWait),conn_belongs(conn){}

    enum class Status{
        ExecAsyncTask,
        Providing,
        FinishWait,
        Done,
    };

    template<typename Filter>
    struct FilterChain{
        FilterChain():_req(nullptr){}
        FilterChain(HttpRequest &req,const std::list<Filter> &filters):_req(&req),_filters(filters){}

        /// @brief execute the filters in sequence and return true if all filters are executed successfully
        template<typename ...FilterArgs>
        bool operator()(FilterArgs&& ...args) const{
            if(_filters.empty())
                return true;

            auto it_filter=_filters.cbegin();
            for(;it_filter!=_filters.cend();++it_filter){
                bool next=false;
                (*it_filter)(*_req,std::forward<FilterArgs>(args)...,[&](){
                    next=true;
                });
                if(!next) break;
            }
            return it_filter==_filters.cend();
        }

        operator bool(){
            return _req&&!_filters.empty();
        }

        void append(Filter filter){
            _filters.emplace_back(std::move(filter));
        }

        void reset(){
            _req=nullptr;
            _filters.clear();
        }

    private:
        HttpRequest *_req;
        std::list<Filter> _filters;
    };

    struct ContentGenerator{
        ContentGenerator(HttpRespData *resp_data):_resp_data(resp_data){}

        using BufAllocator=std::function<uint8_t*(size_t length_expect)>;

        using ProvideDone=std::function<void()>;
        using ProvideAction=std::function<void(ProvideDone is_done)>;

        void set_content_generator(ProvideAction provide_action);

        void set_resp_status(HttpRespData::Status status) noexcept{
            this->_resp_data->status.store(status,std::memory_order_release);
        }

        void try_generate_once(HttpResponseBody &body);

    private:
        ProvideAction _provide_action;
        HttpRespData *_resp_data;
    };

    enum class SendFlag{ NOT_SET,SET,SENT };

    struct HeadFlags{
        SendFlag status_line=SendFlag::NOT_SET;
        SendFlag header_fields=SendFlag::NOT_SET;
    }send_flags;

    HttpResponseHead head;
    HttpResponseBody body;

    std::optional<ContentGenerator> generator;
    FilterChain<HeadFilter> header_filters;
    FilterChain<BodyFilter> body_filters;

    std::atomic<Status> status;

    RxConnection *conn_belongs;

};

/// Internal use API
class HttpRespInternal{
    using Status=HttpRespData::Status;
    using SendFlag=HttpRespData::SendFlag;

public:
    HttpRespInternal(HttpRespData *data):_data(data){}

    /// @brief flush http head and body to connection's output buffer while calling filters
    /// @return false if error occurs
    bool flush(RxChainBuffer &output_buf);

    bool has_block_operation(){
        Status status=_data->status.load(std::memory_order_acquire);
        return status==HttpRespData::Status::ExecAsyncTask||status==HttpRespData::Status::Providing;
    }

    bool has_content_generator(){
        return _data->generator.has_value();
    }

    bool executing_async_task(){
        Status status=_data->status.load(std::memory_order_acquire);
        return status==HttpRespData::Status::ExecAsyncTask;
    }

    void install_head_filters(const HttpRespData::FilterChain<HeadFilter> &filters){
        _data->header_filters=filters;
    }

    void install_body_filters(const HttpRespData::FilterChain<BodyFilter> &filters){
        _data->body_filters=filters;
    }

    void set_status(HttpRespData::Status status){
        _data->status.store(status,std::memory_order_release);
    }

private:
    HttpRespData *_data;
};

}

/// External use API
class HttpResponse{
public:
    using ContentGenerator=detail::HttpRespData::ContentGenerator;
    using ProvideDone=ContentGenerator::ProvideDone;

public:
    HttpResponse(detail::HttpRespData *data):_data(data){}

    HttpResponse& status_code(HttpStatusCode stat_code){
        _data->head.status_line.status_code=stat_code;
        _data->send_flags.status_line=Status::SET;
        return *this;
    }

    HttpResponse& headers(std::string name,std::string val){
        if(_data->send_flags.status_line==Status::NOT_SET){
            throw std::runtime_error("HttpResponse: please set status code before add any header");
        }

        _data->head.header_fields.add(std::move(name),std::move(val));
        _data->send_flags.header_fields=Status::SET;
        return *this;
    }

    HttpResponseBody &body(){
        if(_data->send_flags.status_line==Status::NOT_SET){
            throw std::runtime_error("HttpResponse: please set request line before set body");
        }
        return _data->body;
    }

    void content_generator(ContentGenerator::ProvideAction provide_action){
        if(!_data->generator){
            _data->generator=std::make_optional<ContentGenerator>(this->_data);
        }
        _data->generator->set_content_generator(provide_action);
    }

    /// @brief put http header and the file content as http body into outputbuf
    bool send_file(const std::string &filename,size_t offset=0);
    /// @brief send file immediately to socket without going through filters and outputbuf
    bool send_file_direct(RxConnection *conn,const std::string &filename,size_t offset=0);

    void send_status(HttpStatusCode code){
        const std::string &content=detail::to_http_status_code_str(code);
        this->status_code(code).headers("Content-Length",std::to_string(content.size())).body()<<content;
    }

    void clear();

    bool empty() const{
        return _data->send_flags.status_line==Status::NOT_SET&&_data->send_flags.header_fields==Status::NOT_SET;
    }

    RxConnection *get_connection() const{
        return _data->conn_belongs;
    }

private:
    using Status=detail::HttpRespData::SendFlag;

private:
    detail::HttpRespData *_data;
};

namespace detail {
template<typename ...T>
struct HttpResponseTemplate:protected HttpRespData,public T...{
    HttpResponseTemplate(RxConnection *conn):HttpRespData(conn),T(this)...{}
};

using HttpRespImpl=HttpResponseTemplate<HttpRespInternal,HttpResponse>;
}


struct MakeAsync{
    MakeAsync(Responder responder):_responder(responder){}
    void operator()(HttpRequest &req,HttpResponse &resp);
    Responder _responder;
};

inline static constexpr auto ChunkHeadFilter=[](HttpRequest &,HttpResponseHead &head,Next next){
    head.header_fields.add("Transfer-Encoding","chunked");
    next();
};

inline static constexpr auto ChunkBodyFilter=[](HttpRequest &,HttpResponseBody &body,Next next){
    if(!body.empty()){
        size_t chunk_size=body.readable_size();
        std::string hex_str=Util::int_to_hex(chunk_size);
        body.prepend(hex_str+"\r\n");
        body<<"\r\n";
    }
    else{
        body<<"0\r\n\r\n"; //end of http body
    }
    next();
};

} //namespace Rinx
#endif // HTTPRESPONSEWRITER_H
