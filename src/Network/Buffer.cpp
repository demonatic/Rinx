#include "Buffer.h"
#include "../Util/ObjectAllocator.hpp"
#include <iostream>

#define assert_chunk_index()\
assert(_read_pos>=data()&&_read_pos<=_write_pos&&_write_pos<=_data.data()+N)

template<size_t N>
BufferChunk<N>::BufferChunk():_read_pos(_data.data()),_write_pos(_read_pos),_recycle(false)
{

}

template<size_t N>
size_t BufferChunk<N>::readable_size() const
{
    return _write_pos-_read_pos;
}

template<size_t N>
size_t BufferChunk<N>::writable_size() const
{
    return N-(_write_pos-_data.data());
}

template<size_t N>
void BufferChunk<N>::advance_read(size_t bytes)
{
    _read_pos+=bytes;
    assert_chunk_index();
    if(_read_pos==_write_pos){
        _read_pos=_write_pos=_data.data();
    }
}

template<size_t N>
void BufferChunk<N>::advance_write(size_t bytes)
{
    _write_pos+=bytes;
    assert_chunk_index();
}

template<size_t N>
typename BufferChunk<N>::value_type *BufferChunk<N>::data()
{
    return _data.data();
}

template<size_t N>
uint8_t* BufferChunk<N>::read_pos() const
{
    return _read_pos;
}

template<size_t N>
uint8_t* BufferChunk<N>::write_pos() const
{
    return _write_pos;
}

template<size_t N>
constexpr size_t BufferChunk<N>::get_length()
{
    return N;
}

size_t ChainBuffer::chunk_num() const
{
    return _chunk_list.size();
}

size_t ChainBuffer::total_size() const
{
    return chunk_size*_chunk_list.size();
}

ssize_t ChainBuffer::read_fd(int fd,Rx_Read_Res &res)
{
    char extra_buff[65535];
    std::vector<struct iovec> io_vecs(2);
    if(_chunk_list.empty()){
        tail_push_new_chunk();
    }
    auto tail=get_tail();
    if(tail->writable_size()==0){
        tail_push_new_chunk();
    }
    io_vecs[0].iov_base=tail->write_pos();
    io_vecs[0].iov_len=tail->writable_size();
    io_vecs[1].iov_base=extra_buff;
    io_vecs[1].iov_len=sizeof extra_buff;

    ssize_t read_bytes=RxSock::readv(fd,io_vecs,res);
    if(read_bytes>0){
        size_t bytes=read_bytes;
        if(bytes<tail->writable_size()){
            tail->advance_write(bytes);
        }
        else{
            bytes-=tail->writable_size();
            tail->advance_write(tail->writable_size());
            this->append(extra_buff,bytes);
        }
    }
    return read_bytes;
}

ssize_t ChainBuffer::write_fd(int fd,Rx_Write_Res &res)
{
    std::vector<struct iovec> io_vecs;
    for(auto it_chunk=_chunk_list.begin();it_chunk!=_chunk_list.end();it_chunk++){
        if((*it_chunk)->readable_size()==0){
            break;
        }
        struct iovec vec;
        vec.iov_base=(*it_chunk)->read_pos();
        vec.iov_len=(*it_chunk)->readable_size();
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
    return read_iterator(this,_chunk_list.begin(),_chunk_list.front()->read_pos());
}

ChainBuffer::read_iterator ChainBuffer::readable_end()
{
    return read_iterator(this,_chunk_list.end(),nullptr);
}

ChainBuffer::chunk_iterator ChainBuffer::chunk_begin()
{
    return _chunk_list.begin();
}

ChainBuffer::chunk_iterator ChainBuffer::chunk_end()
{
    return _chunk_list.end();
}

void ChainBuffer::append(char *data, size_t length)
{
    char *pos=data;
    size_t bytes_left=length;
    while(bytes_left>0){
        if(get_tail()->writable_size()==0){
            tail_push_new_chunk();
        }

        if(get_tail()->writable_size()>bytes_left){
            std::copy(pos,pos+bytes_left,get_tail()->write_pos());
            get_tail()->advance_write(bytes_left);
            bytes_left=0;
        }
        else{
            size_t write_size=get_tail()->writable_size();
            std::copy(pos,pos+write_size,get_tail()->write_pos());
            get_tail()->advance_write(write_size);
            pos+=write_size;
            bytes_left-=write_size;
        }
    }
}

ChainBuffer::chunk_ptr ChainBuffer::get_head() const
{
    return _chunk_list.front();
}

ChainBuffer::chunk_ptr ChainBuffer::get_tail() const
{
    return _chunk_list.back();
}

std::unique_ptr<ChainBuffer> ChainBuffer::create_chain_buffer()
{
    auto buff=std::make_unique<ChainBuffer>();
    buff->tail_push_new_chunk();
    return buff;
}

ChainBuffer::chunk_ptr ChainBuffer::new_chunk()
{
    return rx_pool_make_shared<chunk_type>();
}

void ChainBuffer::advance_write(std::list<chunk_ptr>::iterator it_from, size_t bytes)
{
    while(bytes>0&&it_from!=_chunk_list.end()){
        size_t write_space=(*it_from)->writable_size();
        if(write_space<bytes){
            (*it_from)->advance_write(write_space);
            bytes-=write_space;
            it_from++;
        }
        else{
            (*it_from)->advance_write(bytes);
            bytes=0;
        }
    }
}

void ChainBuffer::advance_read(size_t bytes)
{
    auto it_head=_chunk_list.begin();
    while(bytes>0&&it_head!=_chunk_list.end()){
        if((*it_head)->readable_size()>bytes){
            (*it_head)->advance_read(bytes);
            bytes=0;
        }
        else{
            size_t read_space=(*it_head)->readable_size();
            (*it_head)->advance_read(read_space);
            //TODO add low chunk num thresh
            if(this->head_pop_unused_chunk(false)){
                it_head=_chunk_list.begin();
            }
            bytes-=read_space;
        }
    }
}

void ChainBuffer::tail_push_new_chunk()
{
    _chunk_list.emplace_back(new_chunk());
}

bool ChainBuffer::head_pop_unused_chunk(bool force)
{
    if(this->get_head()->readable_size()!=0&&!force){
        return false;
    }
    _chunk_list.pop_front();
    return true;
}
