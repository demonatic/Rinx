#ifndef BUFFER_H
#define BUFFER_H

#include <array>
#include <memory>
#include <list>
#include "../RinxDefines.h"
#include "Socket.h"
#include <iostream>

template<size_t N>
class BufferChunk
{
public:
    using value_type=uint8_t;

    BufferChunk();
    value_type* read_pos() const;
    value_type* write_pos() const;

    size_t readable_size() const;
    size_t writable_size() const;

    void advance_read(size_t bytes);
    void advance_write(size_t bytes);

    value_type *data();
    constexpr size_t get_length();

private:
    std::array<uint8_t,N> _data;
    ///next position of the last bytes processed
    value_type* _read_pos;
    ///next position of the last bytes unprocessed
    value_type* _write_pos;

    bool _recycle;
};

template<class ChunkIteratorType,class ByteType,size_t N>
class BufferReadableIterator;

///buffer最后一块writeable_size可为0
class ChainBuffer{
public:
    static constexpr size_t chunk_size=RX_BUFFER_CHUNK_SIZE;
    using chunk_type=BufferChunk<chunk_size>;
    using chunk_ptr=std::shared_ptr<chunk_type>;
    using chunk_iterator=std::list<chunk_ptr>::iterator;
    using read_iterator=BufferReadableIterator<chunk_iterator,chunk_type::value_type,chunk_size>;

    ChainBuffer()=default;

    size_t chunk_num() const;
    size_t total_size() const;

    ssize_t read_fd(int fd,Rx_Read_Res &res);
    /// write all data in the buffer to fd
    ssize_t write_fd(int fd,Rx_Write_Res &res);

    read_iterator readable_begin();
    read_iterator readable_end();

    chunk_iterator chunk_begin();
    chunk_iterator chunk_end();

    void append(char *data,size_t length);

    chunk_ptr get_head() const;
    chunk_ptr get_tail() const;

    static std::unique_ptr<ChainBuffer> create_chain_buffer();
    static chunk_ptr new_chunk();

    void advance_read(size_t bytes);
    void advance_write(std::list<chunk_ptr>::iterator it_from,size_t bytes);

    void tail_push_new_chunk();
    bool head_pop_unused_chunk(bool force);

private:
    std::list<chunk_ptr> _chunk_list;
};

using RxChainBuffer=ChainBuffer;


template<class ChunkIteratorType,class ByteType,size_t N>
class BufferReadableIterator:public std::iterator<
        std::input_iterator_tag,
        ByteType,
        std::ptrdiff_t,
        ByteType*,
        ByteType&
      >{
public:
    using chunk_type=ChainBuffer::chunk_type;
    using chunk_ptr=ChainBuffer::chunk_ptr;
    using self_type=BufferReadableIterator<ChunkIteratorType,ByteType,N>;

    using difference_type=typename BufferReadableIterator::difference_type;
    using value_type=typename BufferReadableIterator::value_type;
    using reference=typename BufferReadableIterator::reference;

    BufferReadableIterator(ChainBuffer *chain_buf,ChunkIteratorType it_chunk,value_type *cur):
        _chain_buf(chain_buf),_it_chunk(it_chunk),_cur(cur)
    {
        if(it_chunk!=chain_buf->chunk_end()){
            _chunk_start=static_cast<chunk_ptr>(*it_chunk)->read_pos();
            _chunk_end=_chunk_start+static_cast<chunk_ptr>(*it_chunk)->readable_size();
        }
        else{
            _chunk_start=_chunk_end=_cur;
        }
    }

    value_type& operator*() const{
        return *_cur;
    }

    bool operator==(const self_type &other) const{
        return _it_chunk==other._it_chunk&&_cur==other._cur;
    }

    bool operator!=(const self_type &other) const{
        return !this->operator==(other);
    }

    self_type& operator++(){
        if(_cur==nullptr){
            throw std::runtime_error("index out of range");
        }

        _cur++;
        if(_cur==_chunk_end){
            ++_it_chunk;
            if(_it_chunk!=_chain_buf->chunk_end()){
                _chunk_start=static_cast<chunk_ptr>(*_it_chunk)->read_pos();
                _chunk_end=_chunk_start+static_cast<chunk_ptr>(*_it_chunk)->readable_size();
                _cur=_chunk_start;
            }
            else{
                _chunk_start=_chunk_end=_cur=nullptr;
            }
        }
        return *this;
    }

    self_type operator++(int){
        self_type tmp=*this;
        ++(*this);
        return tmp;
    }

    self_type& operator--(){
        if(_cur==_chunk_start){
            if(_it_chunk!=_chain_buf->chunk_begin()){
                --_it_chunk;
                _chunk_start=static_cast<chunk_ptr>(*_it_chunk)->read_pos();
                _chunk_end=_chunk_start+static_cast<chunk_ptr>(*_it_chunk)->readable_size();
                _cur=_chunk_end-1;
            }
            else{
                throw std::runtime_error("index out of range");
            }
        }
        else {
            _cur--;
        }
        return *this;
    }

    self_type operator--(int){
        self_type tmp=*this;
        --(*this);
        return tmp;
    }

    self_type& operator+=(difference_type step){
        if(_cur==nullptr){
            throw std::runtime_error("index out of range");
        }

        difference_type offset=step+_cur-_chunk_start;
        if(offset>=0&&offset<_chunk_end-_chunk_start){
            _cur+=step;
        }
        else if(step>0){
            do{
                difference_type chunk_advance_room=_chunk_end-_cur-1;
                if(step<=chunk_advance_room){
                    _cur+=step;
                    step=0;
                }
                else{
                    step-=chunk_advance_room;
                    ++_it_chunk;
                    if(_it_chunk==_chain_buf->chunk_end()){
                        if(step>1){
                            throw std::runtime_error("index out of range");
                        }
                        else{
                            _chunk_start=_chunk_end=_cur=nullptr;
                            step=0;
                        }
                    }
                    else{
                        _chunk_start=static_cast<chunk_ptr>(*_it_chunk)->read_pos();
                        _chunk_end=_chunk_start+static_cast<chunk_ptr>(*_it_chunk)->readable_size();
                        _cur=_chunk_start;
                        --step;
                    }
                }

            }while(step!=0);

        }
        else if(step<0){
            do{
                difference_type chunk_advance_room=_cur-_chunk_start;
                if(-step<=chunk_advance_room){
                    _cur-=step;
                    step=0;
                }
                else{
                    if(_it_chunk==_chain_buf->chunk_begin()){
                        throw std::runtime_error("index out of range");
                    }
                    step+=chunk_advance_room+1;
                    --_it_chunk;
                    _chunk_start=static_cast<chunk_ptr>(*_it_chunk)->read_pos();
                    _chunk_end=_chunk_start+static_cast<chunk_ptr>(*_it_chunk)->readable_size();
                    _cur=_chunk_end-1;
                }
            }while(step!=0);
        }
    }

    self_type operator+(difference_type n) const{
        self_type tmp=*this;
        return tmp+=n;
    }

    self_type& operator-=(difference_type n){
        return *this+=(-n);
    }

    self_type operator-(difference_type n) const{
        self_type tmp=*this;
        return tmp-=n;
    }

private:
    ChainBuffer *_chain_buf;
    ChunkIteratorType _it_chunk;

    value_type *_chunk_start;
    value_type *_chunk_end;
    value_type *_cur;

};

#endif // BUFFERCHUNK_H
