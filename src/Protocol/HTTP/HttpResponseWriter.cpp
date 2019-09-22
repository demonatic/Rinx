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
    _buff.tail_push_new_chunk();
    long expect_read=std::min(stream_length,_buff.chunk_size);

    long actual_read=ifstream.read(reinterpret_cast<char*>(_buff.writable_pos()),expect_read).gcount();
    if(actual_read<=0)
        return 0;

    _buff.get_tail()->advance_write(actual_read);
    return actual_read;
}
