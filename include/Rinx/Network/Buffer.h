#ifndef BUFFER_H
#define BUFFER_H

#include <sys/mman.h>
#include <array>
#include <memory>
#include <deque>
#include <cstring>
#include "FD.h"
#include "Rinx/Util/ObjectAllocator.hpp"
#include "Rinx/RinxDefines.h"
#include "3rd/NanoLog/NanoLog.hpp"

namespace Rinx {

class BufferRaw{
public:
    using value_type=uint8_t;
    using Ptr=std::shared_ptr<BufferRaw>;

    virtual ~BufferRaw()=default;

    template<typename T,typename ...Args>
    static Ptr create(Args... args){
         return rx_pool_make_shared<T>(std::forward<Args>(args)...);
    }

    value_type* data() const{ return this->_p_data; }
    size_t length() const{ return this->_len; }

protected:
    BufferRaw():_p_data(nullptr),_len(0){}

    value_type *_p_data;
    size_t _len;
};

/// @class provide a buffer of fixed size
template<size_t N=BufferChunkSize>
class BufferFixed:public BufferRaw{
public:
    BufferFixed(){
        _len=N;
        _p_data=_data;
    }
private:
    uint8_t _data[N];
};

class BufferMalloc:public BufferRaw{
public:
    BufferMalloc(size_t length){
        _p_data=static_cast<value_type*>(::malloc(length));
        _len=length;
    }

    ~BufferMalloc(){
        ::free(_p_data);
    }
};

/// @class provide a mmap of a regular file
class BufferFile:public BufferRaw{
public:
    BufferFile(int fd,size_t length){
        _p_data=static_cast<value_type*>(::mmap(nullptr,length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0));
        if(_p_data==MAP_FAILED){
            LOG_WARN<<"create mmap for fd "<<fd<<" failed with len="<<length<<", Reason:"<<errno<<' '<<strerror(errno);
            throw std::bad_alloc();
        }
        _len=length;
    }
    ~BufferFile(){
        ::munmap(_p_data,_len);
    }
};

class ChainBuffer;
class BufferSlice
{
    friend ChainBuffer;
public:
    using value_type=typename BufferRaw::value_type;

    BufferSlice(typename BufferRaw::Ptr buf_raw,size_t start_pos=0,size_t end_pos=0):
        _start_pos(start_pos),_end_pos(end_pos),_buf_ptr(buf_raw)
    {

    }
    value_type* read_pos(){ return data()+_start_pos; }
    value_type* write_pos(){ return data()+_end_pos; }

    size_t readable_size() const{ return _end_pos-_start_pos; }
    size_t writable_size() const{ return _buf_ptr->length()-_end_pos; }

private:
    value_type *data(){ return _buf_ptr->data(); }
    auto raw_buf_sptr(){ return _buf_ptr; }

    void check_index_valid(){
        assert(_start_pos<=_end_pos&&_end_pos<=_buf_ptr->length());
    }

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
private:
    size_t _start_pos;
    size_t _end_pos;
    typename BufferRaw::Ptr _buf_ptr;
};

template<class ChunkIteratorType,class ByteType>
class BufferReadIterator;

///buffer only expand its size when try to write things in it
class ChainBuffer{

private:
    std::deque<BufferSlice> _buf_slice_list;

public:
    using buf_slice_iterator=decltype(_buf_slice_list)::iterator;
    using read_iterator=BufferReadIterator<buf_slice_iterator,BufferSlice::value_type>;
    friend read_iterator;

    ChainBuffer()=default;
    ~ChainBuffer()=default;

    static std::unique_ptr<ChainBuffer> create_chain_buffer();

    void clear();
    bool empty();

    size_t buf_slice_num() const;
    size_t readable_size();

    /// @brief read data as many as possible(no greater than 65535+n) from fd(socket,file...) to buffer
    ssize_t read_from_fd(RxFD fd,RxReadRc &res);

    /// @brief append file of range [offset,offset+length) to buffer using mmap
    /// @return whether file has been successfully read in
    bool read_from_regular_file(RxFD regular_file_fd,size_t length,size_t offset=0);

    /// @brief write all data in buffer to fd(socket,file...)
    ssize_t write_to_fd(RxFD fd,RxWriteRc &res);

    read_iterator begin();
    read_iterator end();

    buf_slice_iterator slice_begin();
    buf_slice_iterator slice_end();

    std::vector<std::pair<uint8_t*,size_t>> get_data();

    /// @brief commit that n_bytes has been consumed by the caller, and the corresponding space in buffer could be freed
    void commit_consume(size_t n_bytes);

    /// @brief slice [it,it+length)
    ChainBuffer slice(read_iterator begin,read_iterator end);

    ///** functions for append data to the buffer **///
    //TODO merge peices
    void append(const char *data,size_t length);

    /// @brief read count bytes from istream to the buffer
    long append(std::istream &istream,long length);

    /// @brief append the buf_slices of parameter buf to current buffer
    void append(ChainBuffer &&buf);

    void append(BufferSlice slice);

    void prepend(BufferSlice slice);
    void prepend(const std::string &data);

    ChainBuffer& operator<<(const char c);
    ChainBuffer& operator<<(const std::string &arg);

    template<size_t N>
    ChainBuffer &operator<<(const char (&arg)[N]){
        append(arg,N-1);
        return *this;
    }

    template<typename Arg>
    typename std::enable_if_t<
        std::is_integral_v<Arg>||std::is_floating_point_v<Arg>,ChainBuffer&> operator<<(const Arg arg)
    {
        std::string str=std::to_string(arg);
        return *this<<str;
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
    void check_need_expand();
};

using RxChainBuffer=ChainBuffer;

template<class BufSliceIteratorType,class ByteType>
class BufferReadIterator:public std::iterator<
        std::input_iterator_tag,
        ByteType,
        std::ptrdiff_t,
        ByteType*,
        ByteType&
    >
{
public:
    using buf_slice_type=BufferSlice;
    using self_type=BufferReadIterator<BufSliceIteratorType,ByteType>;

    using difference_type=typename BufferReadIterator::difference_type;
    using value_type=typename BufferReadIterator::value_type;
    using reference=typename BufferReadIterator::reference;

    friend ChainBuffer;

public:
    BufferReadIterator(ChainBuffer *chain_buf,BufSliceIteratorType it_buf_slice,value_type *cur):
        _chain_buf(chain_buf),_it_buf_slice(it_buf_slice),_p_cur(cur)
    {
        if(it_buf_slice!=chain_buf->slice_end()){
            _p_start=static_cast<buf_slice_type&>(*it_buf_slice).read_pos();
            _p_end=static_cast<buf_slice_type&>(*it_buf_slice).write_pos();
        }
        else{
            _p_start=_p_end=_p_cur;
        }
    }

    value_type operator*() const{
        return *_p_cur;
    }

    bool operator==(const self_type &other) const{
        return _it_buf_slice==other._it_buf_slice&&_p_cur==other._p_cur;
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
            ++_it_buf_slice;

            if(_it_buf_slice!=_chain_buf->slice_end()){
                _p_start=static_cast<buf_slice_type&>(*_it_buf_slice).read_pos();
                _p_end=static_cast<buf_slice_type&>(*_it_buf_slice).write_pos();
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
            if(_it_buf_slice!=_chain_buf->slice_begin()){
                --_it_buf_slice;
                _p_start=static_cast<buf_slice_type&>(*_it_buf_slice).read_pos();
                _p_end=static_cast<buf_slice_type&>(*_it_buf_slice).write_pos();
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
        if(_p_cur==nullptr&&step>0){
            throw std::runtime_error("index out of range");
        }

        difference_type offset=step+_p_cur-_p_start;
        if(offset>=0&&offset<_p_end-_p_start){
            _p_cur+=step;
        }
        else if(step>0){
            do{
                difference_type buf_slice_advance_room=std::max(_p_end-_p_cur-1,0l);

                if(step<=buf_slice_advance_room){
                    _p_cur+=step;
                    break;
                }
                else{
                    step-=buf_slice_advance_room;
                    ++_it_buf_slice;
                    if(_it_buf_slice==_chain_buf->slice_end()){
                        if(step>1){
                            throw std::runtime_error("index out of range");
                        }
                        _p_start=_p_end=_p_cur=nullptr;
                        break;
                    }
                    else{
                        _p_start=static_cast<buf_slice_type&>(*_it_buf_slice).read_pos();
                        _p_end=static_cast<buf_slice_type&>(*_it_buf_slice).write_pos();
                        _p_cur=_p_start;
                        --step;
                    }
                }

            }while(step!=0);
        }
        else if(step<0){
            do{
                difference_type buf_slice_advance_room=std::max(_p_cur-_p_start,0l);
                if(-step<=buf_slice_advance_room){
                    _p_cur-=step;
                    step=0;
                }
                else{
                    if(_it_buf_slice==_chain_buf->slice_begin()){
                        throw std::runtime_error("index out of range");
                    }
                    step+=buf_slice_advance_room+1;
                    --_it_buf_slice;
                    _p_start=static_cast<buf_slice_type&>(*_it_buf_slice).read_pos();
                    _p_end=static_cast<buf_slice_type&>(*_it_buf_slice).write_pos();
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
        if(this->_it_buf_slice==other._it_buf_slice){
            return this->_p_cur-other._p_cur;
        }
        self_type tmp=*this;
        bool flag=false;
        if(tmp._it_buf_slice<other._it_buf_slice||(tmp._it_buf_slice==other._it_buf_slice&&tmp._p_cur<other._p_cur)){
            std::swap(tmp,other);
            flag=true;
        }
        //other<=tmp
        difference_type diff=-(other._p_cur-other._p_start);
        while(other._it_buf_slice!=tmp._it_buf_slice){
            diff+=other._p_end-other._p_start;
            difference_type advance=other._p_end-other._p_cur;
            other+=(advance==0?1:advance);
        }
        difference_type ret=diff+(tmp._p_cur-other._p_cur);
        return flag?-ret:ret;
    }

private:
    ChainBuffer *_chain_buf;
    BufSliceIteratorType _it_buf_slice;

    value_type *_p_start;
    value_type *_p_end;
    value_type *_p_cur;

};

} //namespace Rinx
#endif // BUFFERCHUNK_H
