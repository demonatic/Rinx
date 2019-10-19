#ifndef BUFFER_H
#define BUFFER_H

#include <array>
#include <memory>
#include <list>
#include <cstring>
#include "Socket.h"
#include "../RinxDefines.h"
#include <iostream>

template<size_t N>
struct BufferRaw{
    using value_type=uint8_t;
    using Ptr=std::shared_ptr<BufferRaw>;

    static Ptr create_one();
    std::array<uint8_t,N> raw_data;
};

template<size_t N>
class BufferRef
{
public:
    using value_type=typename BufferRaw<N>::value_type;

    BufferRef(size_t start_pos=0,size_t end_pos=0,typename BufferRaw<N>::Ptr buf_raw=nullptr):
        _start_pos(start_pos),_end_pos(end_pos),_buf_raw(buf_raw==nullptr?BufferRaw<N>::create_one():buf_raw)
    {

    }
    value_type* read_pos(){ return data()+_start_pos; }
    value_type* write_pos(){ return data()+_end_pos; }

    size_t readable_size() const{ return _end_pos-_start_pos; }
    size_t writable_size() const{ return N-_end_pos; }

    #define check_index_valid()\
    assert(_start_pos>=0&&_start_pos<=_end_pos&&_end_pos<=N)

    void advance_read(size_t bytes){
        _start_pos+=bytes;
        check_index_valid();
        if(_start_pos==_end_pos){
            _start_pos=_end_pos=0;
        }
    }
    void advance_write(size_t bytes){
        _end_pos+=bytes;
        check_index_valid();
    }
    value_type *data(){ return _buf_raw->raw_data.data(); }
    auto get_buf_raw_ptr(){ return _buf_raw; }
    constexpr size_t get_length(){ return N; }

private:
    size_t _start_pos;
    size_t _end_pos;
    typename BufferRaw<N>::Ptr _buf_raw;
};

template<class ChunkIteratorType,class ByteType,size_t N>
class BufferReadableIterator;

///buffer only expand its size when try to write things in it
class ChainBuffer{
    static constexpr size_t buf_raw_size=RX_BUFFER_CHUNK_SIZE;
    using buf_ref_type=BufferRef<buf_raw_size>;

private:
    std::list<buf_ref_type> _buf_ref_list;

public:
    using buf_ref_iterator=decltype(_buf_ref_list)::iterator;
    using read_iterator=BufferReadableIterator<buf_ref_iterator,buf_ref_type::value_type,buf_raw_size>;
    friend read_iterator;

    ChainBuffer()=default;
    ~ChainBuffer()=default;

    static std::unique_ptr<ChainBuffer> create_chain_buffer();
    void free();

    size_t buf_ref_num() const;
    size_t capacity() const;
    size_t readable_size();

    ssize_t read_fd(int fd,RxReadRc &res);
    /// write all data in the buffer to fd
    ssize_t write_fd(int fd,RxWriteRc &res);

    read_iterator readable_begin();
    read_iterator readable_end();

    buf_ref_iterator bref_begin();
    buf_ref_iterator bref_end();

    buf_ref_type& get_head();
    buf_ref_type& get_tail();

    void advance_read(size_t bytes);

    /// @brief slice [it,it+length)
    ChainBuffer slice(read_iterator it,size_t length);

    ///** functions for write data to the buffer **///

    void append(const char *data,size_t length);

    /// @brief read count bytes from istream to the buffer
    long append(std::istream &istream,long length);

    /// @brief append the buf_refs of parameter buf to this
    void append(ChainBuffer &buf);

    ChainBuffer& operator<<(const std::string &arg);

    template<typename Arg>
    typename std::enable_if_t<
        std::is_integral_v<Arg>||std::is_floating_point_v<Arg>,ChainBuffer&> operator<<(Arg arg)
    {
        append(reinterpret_cast<char*>(&arg),sizeof(arg));
        return *this;
    }

    template<size_t N>
    ChainBuffer &operator<<(const char (&arg)[N]){
        append(arg,N-1);
        return *this;
    }

    template<typename Arg>
    typename std::enable_if_t<
        std::is_same_v<char *,std::decay_t<Arg>>||
        std::is_same_v<const char *,std::decay_t<Arg>>, ChainBuffer&>
    operator<<(const Arg arg){
        size_t str_len=strlen(arg);
        if(str_len!=0){
           append(arg,str_len);
        }
        return *this;
    }

private:
    void push_buf_ref(buf_ref_type ref={});
    bool pop_unused_buf_ref(bool force=false);

    void check_need_expand();
};

using RxChainBuffer=ChainBuffer;

template<class BufRefIteratorType,class ByteType,size_t N>
class BufferReadableIterator:public std::iterator<
        std::input_iterator_tag,
        ByteType,
        std::ptrdiff_t,
        ByteType*,
        ByteType&
    >
{
public:
    using buf_ref_type=ChainBuffer::buf_ref_type;
    using self_type=BufferReadableIterator<BufRefIteratorType,ByteType,N>;

    using difference_type=typename BufferReadableIterator::difference_type;
    using value_type=typename BufferReadableIterator::value_type;
    using reference=typename BufferReadableIterator::reference;

    friend ChainBuffer;

public:
    BufferReadableIterator(ChainBuffer *chain_buf,BufRefIteratorType it_buf_ref,value_type *cur):
        _chain_buf(chain_buf),_it_buf_ref(it_buf_ref),_p_cur(cur)
    {
        if(it_buf_ref!=chain_buf->bref_end()){
            _p_start=static_cast<buf_ref_type&>(*it_buf_ref).read_pos();
            _p_end=static_cast<buf_ref_type&>(*it_buf_ref).write_pos();
        }
        else{
            _p_start=_p_end=_p_cur;
        }
    }

    value_type operator*() const{
        return *_p_cur;
    }

    bool operator==(const self_type &other) const{
        return _it_buf_ref==other._it_buf_ref&&_p_cur==other._p_cur;
    }

    bool operator!=(const self_type &other) const{
        return !this->operator==(other);
    }

    self_type& operator++(){
        if(_p_cur==nullptr){
            throw std::runtime_error("index out of range");
        }

        _p_cur++;
        if(_p_cur==_p_end){
            ++_it_buf_ref;

            if(_it_buf_ref!=_chain_buf->bref_end()){
                _p_start=static_cast<buf_ref_type&>(*_it_buf_ref).read_pos();
                _p_end=static_cast<buf_ref_type&>(*_it_buf_ref).write_pos();
                _p_cur=_p_start;
            }
            else{
                _p_start=_p_end=_p_cur=nullptr;
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
        if(_p_cur==_p_start){
            if(_it_buf_ref!=_chain_buf->bref_begin()){
                --_it_buf_ref;
                _p_start=static_cast<buf_ref_type&>(*_it_buf_ref).read_pos();
                _p_end=static_cast<buf_ref_type&>(*_it_buf_ref).write_pos();
                _p_cur=_p_end-1;
            }
            else{
                throw std::runtime_error("index out of range");
            }
        }
        else {
            _p_cur--;
        }
        return *this;
    }

    self_type operator--(int){
        self_type tmp=*this;
        --(*this);
        return tmp;
    }

    self_type& operator+=(difference_type step){
        if(_p_cur==nullptr){
            throw std::runtime_error("index out of range");
        }

        difference_type offset=step+_p_cur-_p_start;
        if(offset>=0&&offset<_p_end-_p_start){
            _p_cur+=step;
        }
        else if(step>0){
            do{
                difference_type buf_ref_advance_room=_p_end-_p_cur-1;
                if(step<=buf_ref_advance_room){
                    _p_cur+=step;
                    break;
                }
                else{
                    step-=buf_ref_advance_room;
                    ++_it_buf_ref;
                    if(_it_buf_ref==_chain_buf->bref_end()){
                        if(step>1){
                            throw std::runtime_error("index out of range");
                        }
                        _p_start=_p_end=_p_cur=nullptr;
                        break;
                    }
                    else{
                        _p_start=static_cast<buf_ref_type&>(*_it_buf_ref).read_pos();
                        _p_end=static_cast<buf_ref_type&>(*_it_buf_ref).write_pos();
                        _p_cur=_p_start;
                        --step;
                    }
                }

            }while(step!=0);

        }
        else if(step<0){
            do{
                difference_type buf_ref_advance_room=_p_cur-_p_start;
                if(-step<=buf_ref_advance_room){
                    _p_cur-=step;
                    step=0;
                }
                else{
                    if(_it_buf_ref==_chain_buf->bref_begin()){
                        throw std::runtime_error("index out of range");
                    }
                    step+=buf_ref_advance_room+1;
                    --_it_buf_ref;
                    _p_start=static_cast<buf_ref_type&>(*_it_buf_ref).read_pos();
                    _p_end=static_cast<buf_ref_type&>(*_it_buf_ref).write_pos();
                    _p_cur=_p_end-1;
                }
            }while(step!=0);
        }

        return *this;
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

    difference_type operator-(self_type other) const{
//        std::cout<<"@operator-:"<<"this.start="<<(size_t)_p_start<<" this.cur="<<(size_t)_p_cur<<"  this.end="<<(size_t)_p_end
//                <<"other.start="<<(size_t)other._p_start<<" other.cur="<<(size_t)other._p_cur<<"  other.end="<<(size_t)other._p_end<<std::endl;
        difference_type diff=-(other._p_cur-other._p_start);

        while(other._it_buf_ref!=this->_it_buf_ref){
            diff+=other._p_end-other._p_start;
            other+=other._p_end-other._p_cur;
        }
        difference_type ret=diff+(this->_p_cur-other._p_cur);
        return ret;
    }

private:
    ChainBuffer *_chain_buf;
    BufRefIteratorType _it_buf_ref;

    value_type *_p_start;
    value_type *_p_end;
    value_type *_p_cur;

};

#endif // BUFFERCHUNK_H
