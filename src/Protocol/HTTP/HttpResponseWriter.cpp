#include "HttpResponseWriter.h"
#include <fstream>

HttpResponse::HttpResponse(RxChainBuffer &buff):_output_buff(buff)
{

}

void HttpResponse::buff_commit_head()
{
    _output_buff<<to_http_version_str(this->_version)<<' '<<"200 OK"<<CRLF;//TODO

    for(auto &header:this->_headers){
        _output_buff<<header.first<<": "<<header.second<<CRLF;
    }
    _output_buff<<CRLF;
}


long HttpResponse::buff_commit_istream(std::istream &ifstream, size_t stream_length)
{
    return _output_buff.write_istream(ifstream,stream_length);
}

