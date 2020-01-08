#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "../../Network/Buffer.h"
#include "../../Network/Connection.h"
#include "HttpDefines.h"
#include "HttpRequest.h"
#include <queue>

class HttpResponse;
class RxProtoHttp1Processor;

/// callable objects to generate response
using Responder=std::function<void(HttpRequest &req,HttpResponse &resp)>;

/// callable objects to filter response
using Next=std::function<void()>;
using HeadFilter=std::function<void(HttpRequest &req,HttpResponseHead &head,Next next)>;
using BodyFilter=std::function<void(HttpRequest &req,HttpResponseBody &body,Next next)>;

struct HttpRespData{
    HttpRespData(RxConnection *conn):head{{HttpStatusCode::OK,HttpVersion::VERSION_1_1},{}},generator(nullptr),conn_belongs(conn){}

    template<typename Filter>
    struct FilterChain{  //TODO add append interface
        FilterChain():_filters(nullptr){}
        FilterChain(HttpRequest &req,const std::list<Filter> &filters):_req(&req),_filters(&filters){}

        /// @brief execute the filters in sequence and return true if all filters are executed successfully
        template<typename ...FilterArgs>
        bool operator()(FilterArgs&& ...args) const{
            if(!_filters)
                return true;

            auto it_filter=_filters->cbegin();
            for(;it_filter!=_filters->cend();++it_filter){
                bool next=false;
                (*it_filter)(*_req,std::forward<FilterArgs>(args)...,[&](){
                    next=true;
                });
                if(!next) break;
            }
            return it_filter==_filters->cend();
        }
        operator bool(){
            return _req&&_filters;
        }
    private:
        HttpRequest *_req;
        const std::list<Filter> *_filters;
    };

    struct ContentGenerator:public std::enable_shared_from_this<ContentGenerator>{
        ContentGenerator(RxConnection *conn):_status(Status::Done),_conn_belongs(conn){}
        using BufAllocator=std::function<uint8_t*(size_t length_expect)>;
        using ProvideDone=std::function<void()>;
        using AsyncTask=std::function<void()>;

        enum class Status{
            ExecAsyncTask,
            Providing,
            Done,
        };
        using ProvideAction=std::function<void(BufAllocator allocator,ProvideDone is_done)>;

        void set_content_provider(ProvideAction provide_action);
        void set_async_content_provider(AsyncTask async_task,ProvideAction provide_action);

        Status get_status() const{ return _status; }
        bool is_done(){return get_status()==Status::Done;}
        void set_status(Status status) noexcept{ this->_status=status; }

        void try_provide_once(HttpResponseBody &body);

    private:
        Status _status;
        ProvideAction _provide_action;
        RxConnection *_conn_belongs;
    };

    enum class ComponentStatus{ NOT_SET,SET,SENT };

    struct HeadStatus{
        ComponentStatus status_line=ComponentStatus::NOT_SET;
        ComponentStatus header_fields=ComponentStatus::NOT_SET;
    }stat;

    HttpResponseHead head;
    HttpResponseBody body;

    std::shared_ptr<ContentGenerator> generator;
    FilterChain<HeadFilter> header_filters;
    FilterChain<BodyFilter> body_filters;

    RxConnection *conn_belongs; //TODO owner
};

/// Internal use API
class HttpRespInternal{
     using Status=HttpRespData::ComponentStatus;
public:
    HttpRespInternal(HttpRespData *data):_data(data){}

    /// @brief flush http head and body to connection's output buffer while calling filters
    /// @return false if error occurs
    bool flush(RxChainBuffer &output_buf);

    bool has_block_operation(){
        return _data->generator&&!_data->generator->is_done();
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
        _data->stat.status_line=Status::SET;
        return *this;
    }

    HttpResponse& headers(std::string name,std::string val){
        if(_data->stat.status_line==Status::NOT_SET){
            throw std::runtime_error("HttpResponse: please set status code before add any header");
        }

        _data->head.header_fields.add(std::move(name),std::move(val));
        _data->stat.header_fields=Status::SET;
        return *this;
    }

    HttpResponseBody &body(){
        if(_data->stat.status_line==Status::NOT_SET||_data->stat.header_fields==Status::NOT_SET){
            throw std::runtime_error("HttpResponse: please set request line and headers before set body");
        }
        return _data->body;
    }

    void content_provider(ContentGenerator::ProvideAction provide_action){
        if(!_data->generator){
            _data->generator=std::make_shared<ContentGenerator>(_data->conn_belongs);
        }
        _data->generator->set_content_provider(provide_action);
    }

    void async_content_provider(ContentGenerator::AsyncTask async_task,ContentGenerator::ProvideAction provide_action){
        if(!_data->generator){
            _data->generator=std::make_shared<ContentGenerator>(_data->conn_belongs);
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
        return _data->stat.status_line==Status::NOT_SET||_data->stat.header_fields==Status::NOT_SET;
    }

private:
    using Status=HttpRespData::ComponentStatus;

private:
    HttpRespData *_data;
};

template<typename ...T>
struct HttpResponseTemplate:protected HttpRespData,public T...{
    HttpResponseTemplate(RxConnection *conn):HttpRespData(conn),T(this)...{}
};

using HttpRespImpl=HttpResponseTemplate<HttpRespInternal,HttpResponse>;


#endif // HTTPRESPONSEWRITER_H
