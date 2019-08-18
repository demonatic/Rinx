#include "HttpParser.h"

ConsumeRes StateRequestLine::consume(iterable_bytes iterable, std::any &request)
{
    std::cout<<"@request line comsume"<<std::endl;
    ConsumeRes ret;
    iterable([&](byte_t const& byte)->bool{
        std::cout<<"data="<<(int)byte<<" ";
        ret.event=std::nullopt;
        ret.next_super_state.emplace(std::pair{1,nullptr});
        return false;
    });
    return ret;
}



ConsumeRes StateHeader::consume(iterable_bytes iterable, std::any &request)
{
    std::cout<<"@header comsume"<<std::endl;
    ConsumeRes ret;
    iterable([&](byte_t const& byte)->bool{
        std::cout<<"data="<<(int)byte<<" ";
        ret.event=std::nullopt;
        ret.next_super_state.emplace(std::pair{0,nullptr});
        return false;
    });
    return ret;
}
