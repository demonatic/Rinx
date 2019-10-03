#include "HttpRequestHandler.h"
#include "HttpResponse.h"
#include "HttpRequest.h"
#include "../../3rd/NanoLog/NanoLog.h"
#include <exception>


//namespace fs=std::filesystem;
//const std::filesystem::path StaticHttpRequestHandler::_web_root_path="/home/demonaitc/";
//const std::filesystem::path StaticHttpRequestHandler::_default_page="short.txt";

//StaticHttpRequestHandler::StaticHttpRequestHandler():_request_file(nullptr),_file_size(0),_consumed_size(0)
//{

//}

//StaticHttpRequestHandler::~StaticHttpRequestHandler()
//{

//}

//void StaticHttpRequestHandler::on_header_fetched(HttpRequest &request)
//{
//    if(serve_file(request.uri,request.conn_belongs->get_output_buf())){
//        RxWriteRc res;
//        ssize_t send_bytes=request.conn_belongs->send(res);
//        //TOALTER
//        request.clear_for_next_request();
//    }
//}

//void StaticHttpRequestHandler::on_part_of_body(HttpRequest &request)
//{

//}

//void StaticHttpRequestHandler::on_body_finished(HttpRequest &request)
//{

//}

//bool StaticHttpRequestHandler::start_serve_file(const std::filesystem::path &uri,RxChainBuffer &buff)
//{
//    HttpResponseWriter response_writer(buff);

//    if(!_request_file){
//        fs::path authentic_path="/home/demonaitc/nginx.html";
////        try {
//////            std::cout<<"@web root path="<<_web_root_path<<std::endl;
//////            std::cout<<"@uri="<<uri<<std::endl;
////            authentic_path=_web_root_path/uri;
//////            std::cout<<"@authentic path="<<authentic_path<<std::endl;
////            //check weather authentic_path is in root path
//////            if(std::distance(_web_root_path.begin(),_web_root_path.end())>std::distance(authentic_path.begin(),authentic_path.end())
//////                    ||!std::equal(_web_root_path.begin(),_web_root_path.end(),authentic_path.begin())){
//////                throw std::invalid_argument("request uri must be in web root path");
//////            }
////            if(fs::is_directory(authentic_path))
////                authentic_path/=_default_page;

////            if(!fs::exists(authentic_path))
////                throw std::runtime_error("file not found");

////        } catch (std::exception &e) {
////             LOG_INFO<<"cannot serve file: "<<uri<<" exception:"<<e.what()<<" with authentic path="<<authentic_path;
////             return false;
////        }

//        _request_file=std::make_unique<std::ifstream>(authentic_path,std::ifstream::ate|std::ifstream::in|std::ifstream::binary);
//        long read_cur=0;
//        if(!_request_file->is_open()||(read_cur=_request_file->tellg())==-1){
//            LOG_WARN<<"Fail when fstream open and read file:"<<authentic_path;
//            return false;
//        }
//        _file_size=static_cast<size_t>(read_cur);
//        _request_file->seekg(std::ios::beg);

//        HttpResponse resp;
//        resp.set_response_line(HttpStatusCode::OK,HttpVersion::VERSION_1_1);
//        resp.put_header("Content-Length",std::to_string(_file_size));
////        resp.put_header("Content-Length",std::to_string(1));
//        resp.put_header("Connection","keep-alive");
//        resp.put_header("Content-Type",get_mimetype_by_filename(authentic_path));
//        response_writer.write_http_head(resp);
////        buff<<"1";
//    }


//    //    std::cout<<"buff write in="<<buff_write_in<<std::endl;
////    std::cout<<"file served"<<std::endl;
//    return true;
//}

//bool StaticHttpRequestHandler::resume_serve_file(RxChainBuffer &buff)
//{
//    size_t send_size=std::min(_file_size-_consumed_size,one_round_length_limit);
//    long buff_write_in=response_writer.write_fstream(*_request_file,send_size);

//    if(buff_write_in==0){
//        return false;
//    }
//    _consumed_size+=buff_write_in;
//    if(this->is_file_send_done()){
//       this->reset();
//    }
//}

//bool StaticHttpRequestHandler::is_file_send_done()
//{
//    return _consumed_size>=_file_size;
//}

//void StaticHttpRequestHandler::reset()
//{
//    _request_file->close();
//    _request_file.reset();
////        std::cout<<"release request file"<<std::endl;
//    _file_size=_consumed_size=0;
//}
