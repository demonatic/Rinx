#include "HttpRequestHandler.h"
#include "HttpResponseWriter.h"
#include "HttpRequest.h"
#include "../../3rd/NanoLog/NanoLog.h"
#include <exception>


namespace fs=std::filesystem;
const std::filesystem::path StaticHttpRequestHandler::_web_root_path="/home/demonaitc/";
const std::filesystem::path StaticHttpRequestHandler::_default_page="short.txt";

StaticHttpRequestHandler::StaticHttpRequestHandler():_request_file(nullptr),_file_size(0),_consumed_size(0)
{

}

StaticHttpRequestHandler::~StaticHttpRequestHandler()
{

}

void StaticHttpRequestHandler::on_header_fetched(HttpRequest &request)
{
    if(serve_file(request.uri,request.conn_belongs->get_output_buf())){
        Rx_Write_Res res;
        request.conn_belongs->send(res);
        //TOALTER
        request.reset();
    }
}

void StaticHttpRequestHandler::on_part_of_body(HttpRequest &request)
{

}

void StaticHttpRequestHandler::on_body_finished(HttpRequest &request)
{

}

bool StaticHttpRequestHandler::serve_file(const std::filesystem::path &uri,RxChainBuffer &buff)
{
    HttpResponseWriter response_writer(buff);

    if(!_request_file){
        fs::path authentic_path="/home/demonaitc/8KB.txt";
//        try {
//            std::cout<<"@web root path="<<_web_root_path<<std::endl;
//            std::cout<<"@uri="<<uri<<std::endl;
//            authentic_path=_web_root_path/uri;
//            std::cout<<"@authentic path="<<authentic_path<<std::endl;
//            //check weather authentic_path is in root path
////            if(std::distance(_web_root_path.begin(),_web_root_path.end())>std::distance(authentic_path.begin(),authentic_path.end())
////                    ||!std::equal(_web_root_path.begin(),_web_root_path.end(),authentic_path.begin())){
////                throw std::invalid_argument("request uri must be in web root path");
////            }
//            if(fs::is_directory(authentic_path))
//                authentic_path/=_default_page;

//            if(!fs::exists(authentic_path))
//                throw std::runtime_error("file not found");

//        } catch (std::exception &e) {
//             LOG_INFO<<"cannot serve file: "<<uri<<" exception:"<<e.what()<<" with authentic path="<<authentic_path;
//             return false;
//        }

        _request_file=std::make_unique<std::ifstream>(authentic_path,std::ifstream::ate|std::ifstream::in|std::ifstream::binary);
        long read_cur=0;
        if(!_request_file->is_open()||(read_cur=_request_file->tellg())==-1){
            LOG_WARN<<"Fail when fstream open and read file:"<<authentic_path;
            return false;
        }
        _file_size=static_cast<size_t>(read_cur);
        _request_file->seekg(std::ios::beg);

        HttpResponse resp;
        resp.set_response_line(HttpStatusCode::OK,HttpVersion::VERSION_1_1);
        resp.put_header("Content-Length",std::to_string(_file_size));
        resp.put_header("Connection","keep-alive");
        resp.put_header("Content-Type",get_mimetype_by_filename(authentic_path));
        response_writer.write_http_head(resp);
    }

    long buff_write_in=response_writer.write_fstream(*_request_file,_file_size-_consumed_size);
    if(!buff_write_in){
        return false;
    }
    _consumed_size+=buff_write_in;
    if(_consumed_size==_file_size){
        _request_file->close();
        _request_file.release();
        _file_size=_consumed_size=0;
    }
//    std::cout<<"file served"<<std::endl;
    return true;
}
