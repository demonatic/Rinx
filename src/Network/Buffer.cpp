#include "Buffer.h"
#include "../Util/ObjectAllocator.hpp"
#include <iostream>

template class BufferRef<ChainBuffer::buf_raw_size>;


template<size_t N>
typename BufferRaw<N>::Ptr BufferRaw<N>::create_one()
{
      return rx_pool_make_shared<BufferRaw<N>>();
}

size_t ChainBuffer::buf_ref_num() const
{
    return _buf_ref_list.size();
}

size_t ChainBuffer::capacity() const
{
    return buf_raw_size*buf_ref_num();
}

size_t ChainBuffer::readable_size()
{
    return readable_end()-readable_begin();
}

ssize_t ChainBuffer::read_fd(int fd,RxReadRc &res)
{
    this->check_need_expand();

    char extra_buff[65535];
    std::vector<struct iovec> io_vecs(2);
    buf_ref_type &tail=get_tail();
    io_vecs[0].iov_base=tail.write_pos();
    io_vecs[0].iov_len=tail.writable_size();
    io_vecs[1].iov_base=extra_buff;
    io_vecs[1].iov_len=sizeof extra_buff;

    ssize_t read_bytes=RxSock::readv(fd,io_vecs,res);
    if(read_bytes>0){
        size_t bytes=read_bytes;
        if(bytes<tail.writable_size()){
            tail.advance_write(bytes);
        }
        else{
            bytes-=tail.writable_size();
            tail.advance_write(tail.writable_size());
            this->append(extra_buff,bytes);
        }
    }
    return read_bytes;
}

ssize_t ChainBuffer::write_fd(int fd,RxWriteRc &res)
{
    std::vector<struct iovec> io_vecs;
    for(auto it_chunk=_buf_ref_list.begin();it_chunk!=_buf_ref_list.end();++it_chunk){
        buf_ref_type &buf_ref=(*it_chunk);
        if((*it_chunk).readable_size()==0){
            break;
        }
        struct iovec vec;
        vec.iov_base=buf_ref.read_pos();
        vec.iov_len=buf_ref.readable_size();
        io_vecs.emplace_back(std::move(vec));
    }

    ssize_t bytes=RxSock::writev(fd,io_vecs,res);
    if(bytes>0){
        this->advance_read(bytes);
    }
    return bytes;
}

ChainBuffer::read_iterator ChainBuffer::readable_begin()
{
    return read_iterator(this,_buf_ref_list.begin(),_buf_ref_list.front().read_pos());
}

ChainBuffer::read_iterator ChainBuffer::readable_end()
{
    return read_iterator(this,_buf_ref_list.end(),nullptr);
}

ChainBuffer::buf_ref_iterator ChainBuffer::bref_begin()
{
    return _buf_ref_list.begin();
}

ChainBuffer::buf_ref_iterator ChainBuffer::bref_end()
{
    return _buf_ref_list.end();
}

void ChainBuffer::append(const char *data, size_t length)
{
    const char *pos=data;
    size_t bytes_left=length;

    while(bytes_left>0){
        this->check_need_expand();

        buf_ref_type &tail=this->get_tail();
        if(tail.writable_size()>bytes_left){
            std::copy(pos,pos+bytes_left,tail.write_pos());
            tail.advance_write(bytes_left);
            bytes_left=0;
        }
        else{
            size_t write_size=tail.writable_size();
            std::copy(pos,pos+write_size,tail.write_pos());
            tail.advance_write(write_size);
            pos+=write_size;
            bytes_left-=write_size;
        }
    }
}

long ChainBuffer::append(std::istream &istream,long length)
{
    long total_read=0;

    while(length>0){
        this->check_need_expand();
        long try_to_read=std::min(static_cast<size_t>(length),get_tail().writable_size());
        istream.read(reinterpret_cast<char*>(get_tail().write_pos()),try_to_read).gcount();
        long actual_read=istream.gcount();
        if(actual_read<=0)
            break;

        get_tail().advance_write(actual_read);
        total_read+=actual_read;
        length-=actual_read;
    }

    return total_read;
}

void ChainBuffer::append(ChainBuffer &buf)
{
    for(buf_ref_iterator it=buf.bref_begin();it!=buf.bref_end();++it){
        this->_buf_ref_list.emplace_back(std::move((*it)));
    }
    buf.free();
}


ChainBuffer::buf_ref_type& ChainBuffer::get_head()
{
    return _buf_ref_list.front();
}

ChainBuffer::buf_ref_type& ChainBuffer::get_tail()
{
    return _buf_ref_list.back();
}

std::unique_ptr<ChainBuffer> ChainBuffer::create_chain_buffer()
{
    return std::make_unique<ChainBuffer>();
}

void ChainBuffer::free()
{
    _buf_ref_list.clear();
}

void ChainBuffer::advance_read(size_t bytes)
{
    auto it_head=_buf_ref_list.begin();
    while(bytes>0&&it_head!=_buf_ref_list.end()){
        buf_ref_type &buf_ref=(*it_head);
        if(buf_ref.readable_size()>bytes){
            buf_ref.advance_read(bytes);
            bytes=0;
        }
        else{
            size_t read_space=buf_ref.readable_size();
            buf_ref.advance_read(read_space);
            //TODO add low chunk num thresh
            if(this->pop_unused_buf_ref()){
                it_head=_buf_ref_list.begin();
            }
            bytes-=read_space;
        }
    }
}

ChainBuffer ChainBuffer::slice(ChainBuffer::read_iterator it, size_t length)
{
    ChainBuffer sliced;
    while(length){
        size_t ref_len=std::min(static_cast<size_t>(it._p_end-it._p_cur),length);
        size_t start_pos=static_cast<size_t>(it._p_cur-it._p_start);
        sliced.push_buf_ref(buf_ref_type(start_pos,start_pos+ref_len,it._it_buf_ref->get_buf_raw_ptr()));
        length-=ref_len;
        it+=ref_len;
    }
    return sliced;
}

void ChainBuffer::push_buf_ref(buf_ref_type ref)
{
    _buf_ref_list.emplace_back(std::move(ref));
}

bool ChainBuffer::pop_unused_buf_ref(bool force)
{
    if(this->buf_ref_num()&&this->get_head().readable_size()!=0&&!force){
        return false;
    }
    _buf_ref_list.pop_front();
    return true;
}

void ChainBuffer::check_need_expand()
{
    if(!buf_ref_num()||get_tail().writable_size()==0){
        push_buf_ref();
    }
}

ChainBuffer &ChainBuffer::operator<<(const std::string &arg)
{
    append(arg.c_str(),arg.length());
    return *this;
}

