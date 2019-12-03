#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "../../Network/Buffer.h"
#include "../../Network/Connection.h"
#include "HttpDefines.h"
#include "HttpReqRouter.h"
#include <queue>

//struct FilterChain;
class RxProtoHttp1Processor;

struct ContentGenerator:public std::enable_shared_from_this<ContentGenerator>{
    ContentGenerator(RxConnection *conn):_status(Status::Done),_conn_belongs(conn){

    }
    using BufAllocator=std::function<uint8_t*(size_t length_expect)>;
    using ProvideDone=std::function<void()>;

    enum class Status{
        ExecAsyncTask,
        Providing,
        Done,
    };
    using ProvideAction=std::function<void(BufAllocator allocator,ProvideDone done)>;

    void try_provide_once(HttpResponseBody &body);

    void set_async_content_provider(std::function<void()> async_task,ProvideAction provide_action);
    void set_content_provider(ProvideAction provide_action);

    Status get_status() const{ return _status; }
    bool done(){return get_status()==Status::Done;}
    void set_status(Status status) noexcept{ this->_status=status; }

    Status _status;
    ProvideAction _provide_action;
    RxConnection *_conn_belongs;
};

class HttpResponse{
    friend class HttpRouter;
    friend class RxProtoHttp1Processor;
    friend struct ChainFilter; //TODO

public:
    HttpResponse(RxConnection *conn);

    HttpResponse& status_code(HttpStatusCode stat_code){
        HttpStatusLine &stat_line=_status_line.first;
        stat_line.status_code=stat_code;
        _status_line.second=ComponentStatus::SET;
        return *this;
    }

    HttpResponse& headers(std::string name,std::string val){
        if(_status_line.second==ComponentStatus::NOT_SET){
            throw std::runtime_error("HttpResponse: please set status code before add any header");
        }

        _headers.first.add(std::move(name),std::move(val));
        _headers.second=ComponentStatus::SET;
        return *this;
    }

    HttpResponseBody &body(){
        if(_status_line.second==ComponentStatus::NOT_SET||_headers.second==ComponentStatus::NOT_SET){
            throw std::runtime_error("HttpResponse: please set request line and headers before set body");
        }
        return _body;
    }

    /// @brief put http header and the file content as http body into outputbuf
    bool send_file(const std::string &filename,size_t offset=0);
    /// @brief send file immediately to socket
    bool send_file_direct(RxConnection *conn,const std::string &filename,size_t offset=0);

    void send_status(HttpStatusCode code){
        this->status_code(code).headers("Content-Length","0");
    }

    void clear();
    bool empty() const;

private:
    /// @brief flush http head and body to connection's output buffer
    /// @return whether there is data been flushed
    void flush(RxChainBuffer &output_buf);

    bool has_block_operation(){
        return _gen&&!_gen->done();
    }

    void install_header_filters(const ChainFilter &filters){
        this->_header_filters=filters;
    }

    void install_body_filters(const ChainFilter &filters){
        this->_body_filters=filters;
    }

private:
    enum class ComponentStatus{ NOT_SET,SET,SENT };

    std::pair<HttpStatusLine,ComponentStatus> _status_line;
    std::pair<HttpHeaderFields,ComponentStatus> _headers;
    HttpResponseBody _body;

private:
    std::shared_ptr<ContentGenerator> _gen;
    ChainFilter _header_filters;
    ChainFilter _body_filters;
};


#endif // HTTPRESPONSEWRITER_H
