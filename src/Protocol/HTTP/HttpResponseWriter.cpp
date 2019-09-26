#include "HttpResponseWriter.h"
#include <fstream>

HttpResponseWriter::HttpResponseWriter(RxChainBuffer &buff):_buff(buff)
{

}

void HttpResponseWriter::write_http_head(HttpResponse &response)
{
    _buff<<to_http_version_str(response._version)<<' '<<"200 OK"<<CRLF;//TODO

    for(auto &header:response._headers){
        _buff<<header.first<<": "<<header.second<<CRLF;
    }
    _buff<<CRLF; //end of head
}

long HttpResponseWriter::write_fstream(std::ifstream &ifstream,size_t stream_length)
{
    return _buff.write_istream(ifstream,stream_length);
}
