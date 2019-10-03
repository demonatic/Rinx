#include "HttpResponse.h"
#include <fstream>

HttpResponse::HttpResponse(RxChainBuffer &output_buff):_output_buff(output_buff),_status_code(HttpStatusCode::OK),_version(HttpVersion::VERSION_1_1)
{

}

HttpResponse::HttpResponseBody &HttpResponse::body()
{
    return _body;
}

void HttpResponse::flush()
{
    _output_buff<<to_http_version_str(this->_version)<<' '<<"200 OK"<<CRLF;//TODO

    for(auto &header:this->_headers){
        _output_buff<<header.first<<": "<<header.second<<CRLF;
    }
    _output_buff<<CRLF;
    _output_buff.append(_body);
}


long HttpResponse::body_append_istream(std::istream &ifstream, size_t stream_length)
{
    return _body.append(ifstream,stream_length);
}

