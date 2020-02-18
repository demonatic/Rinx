#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "Network/Buffer.h"
#include "Network/Connection.h"
#include "HttpDefines.h"
#include "HttpRequest.h"
#include <queue>

namespace Rinx {

class HttpResponse;
class RxProtoHttp1Processor;

/// callable objects to generate response
using Responder=std::function<void(HttpRequest &req,HttpResponse &resp)>;

/// callable objects to filter response
using Next=std::function<void()>;
using HeadFilter=std::function<void(HttpRequest &req,HttpResponseHead &head,Next next)>;
using BodyFilter=std::function<void(HttpRequest &req,HttpResponseBody &body,Next next)>;

struct HttpRespData{
    HttpRespData(RxConnection *conn):head{{HttpStatusCode::OK,HttpVersion::VERSION_1_1},{}},
        generator(nullptr),status(Status::FinishWait),conn_belongs(conn){}

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

    private:
        HttpRequest *_req;
        std::list<Filter> _filters;
    };

    struct ContentGenerator:public std::enable_shared_from_this<ContentGenerator>{
        ContentGenerator(HttpRespData *resp_data):_resp_data(resp_data){}

        using BufAllocator=std::function<uint8_t*(size_t length_expect)>;
        using AsyncTask=std::function<void()>;

        using ProvideDone=std::function<void()>;
        using ProvideAction=std::function<void(BufAllocator allocator,ProvideDone is_done)>;

        void set_content_provider(ProvideAction provide_action);
        void set_async_content_provider(AsyncTask async_task,ProvideAction provide_action);

        void set_resp_status(HttpRespData::Status status) noexcept{ this->_resp_data->status=status; }

        void try_provide_once(HttpResponseBody &body);

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

    std::shared_ptr<ContentGenerator> generator;
    FilterChain<HeadFilter> header_filters;
    FilterChain<BodyFilter> body_filters;

    Status status;

    RxConnection *conn_belongs;

};

/// Internal use API
class HttpRespInternal{
     using Status=HttpRespData::SendFlag;
public:
    HttpRespInternal(HttpRespData *data):_data(data){}

    /// @brief flush http head and body to connection's output buffer while calling filters
    /// @return false if error occurs
    bool flush(RxChainBuffer &output_buf);

    bool has_block_operation(){
        return _data->generator&&!(_data->status==HttpRespData::Status::FinishWait);
    }

    void install_head_filters(const HttpRespData::FilterChain<HeadFilter> &filters){
        _data->header_filters=filters;
    }

    void install_body_filters(const HttpRespData::FilterChain<BodyFilter> &filters){
        _data->body_filters=filters;
    }

private:
    HttpRespData *_data;
};

/// External use API
class HttpResponse{
public:
    using ContentGenerator=HttpRespData::ContentGenerator;
    using BufAllocator=ContentGenerator::BufAllocator;
    using ProvideDone=ContentGenerator::ProvideDone;

public:
    HttpResponse(HttpRespData *data):_data(data){}

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

    void content_provider(ContentGenerator::ProvideAction provide_action){
        if(!_data->generator){
            _data->generator=std::make_shared<ContentGenerator>(this->_data);
        }
        _data->generator->set_content_provider(provide_action);
    }

    void async_content_provider(ContentGenerator::AsyncTask async_task,ContentGenerator::ProvideAction provide_action){
        if(!_data->generator){
            _data->generator=std::make_shared<ContentGenerator>(this->_data);
        }
        _data->generator->set_async_content_provider(async_task,provide_action);
    }

    /// @brief put http header and the file content as http body into outputbuf
    bool send_file(const std::string &filename,size_t offset=0);
    /// @brief send file immediately to socket without going through filters and outputbuf
    bool send_file_direct(RxConnection *conn,const std::string &filename,size_t offset=0);

    void send_status(HttpStatusCode code){
        const std::string &content=to_http_status_code_str(code);
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
    using Status=HttpRespData::SendFlag;

private:
    HttpRespData *_data;
};

template<typename ...T>
struct HttpResponseTemplate:protected HttpRespData,public T...{
    HttpResponseTemplate(RxConnection *conn):HttpRespData(conn),T(this)...{}
};

using HttpRespImpl=HttpResponseTemplate<HttpRespInternal,HttpResponse>;

inline static constexpr auto ChunkHeadFilter=[](HttpRequest &,HttpResponseHead &head,Next next){
    std::cout<<"@ChunkHeadFilter"<<std::endl;
    head.header_fields.add("Transfer-Encoding","chunked");
    next();
};

inline static constexpr auto ChunkBodyFilter=[](HttpRequest &req,HttpResponseBody &body,Next next){
    std::cout<<"@ChunkBodyFilter"<<std::endl;
    if(!body.empty()){ //end of http body
        size_t chunk_size=body.readable_size();
        std::string hex_str=Util::int_to_hex(chunk_size);
        body.prepend(hex_str+"\r\n");
        std::cout<<"readable 1="<<body.readable_size()<<std::endl;
        body<<"\r\n";
        std::cout<<"readable 2="<<body.readable_size()<<std::endl;
    }
    else{
        std::cout<<"@send 0 chunk"<<std::endl;
        body<<"0\r\n\r\n";
    }
    next();
};

} //namespace Rinx
#endif // HTTPRESPONSEWRITER_H
