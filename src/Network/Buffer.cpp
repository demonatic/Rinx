#include "Rinx/Network/Buffer.h"
#include "Rinx/Util/Util.h"

namespace Rinx {

ssize_t ChainBuffer::read_from_fd(RxFD fd,RxReadRc &res)
{
    this->check_need_expand();

    char extra_buff[65535];
    std::vector<struct iovec> io_vecs(2);
    BufferSlice &tail=_buf_slice_list.back();
    io_vecs[0].iov_base=tail.write_pos();
    io_vecs[0].iov_len=tail.writable_size();
    io_vecs[1].iov_base=extra_buff;
    io_vecs[1].iov_len=sizeof extra_buff;

    ssize_t read_bytes=FDHelper::Stream::readv(fd,io_vecs,res);
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

bool ChainBuffer::read_from_regular_file(RxFD regular_file_fd, size_t length, size_t offset)
{
    try{
        auto buf_file=BufferRaw::create<BufferFile>(regular_file_fd,length);
        BufferSlice file(buf_file,offset,length);
        this->_buf_slice_list.emplace_back(std::move(file));
    }
    catch(std::bad_alloc &e){
        LOG_WARN<<"err when read from regular file "<<regular_file_fd.raw<<' '<<e.what();
        return false;
    }
    return true;
}

ssize_t ChainBuffer::write_to_fd(RxFD fd,RxWriteRc &res)
{
    std::vector<struct iovec> io_vecs;
    for(auto it_chunk=_buf_slice_list.begin();it_chunk!=_buf_slice_list.end();++it_chunk){
        BufferSlice &buf_slice=(*it_chunk);
        if((*it_chunk).readable_size()==0){
            break;
        }
        struct iovec vec;
        vec.iov_base=buf_slice.read_pos();
        vec.iov_len=buf_slice.readable_size();
        io_vecs.emplace_back(std::move(vec));
    }

    ssize_t bytes=FDHelper::Stream::writev(fd,io_vecs,res);
    if(bytes>0){
        this->commit_consume(bytes);
    }
    return bytes;
}

void ChainBuffer::append(const char *data, size_t length)
{
    const char *pos=data;
    size_t bytes_left=length;

    while(bytes_left>0){
        this->check_need_expand();

        BufferSlice &tail=this->_buf_slice_list.back();
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
        long try_to_read=std::min(static_cast<size_t>(length),_buf_slice_list.back().writable_size());
        istream.read(reinterpret_cast<char*>(_buf_slice_list.back().write_pos()),try_to_read).gcount();
        long actual_read=istream.gcount();
        if(actual_read<=0)
            break;

        _buf_slice_list.back().advance_write(actual_read);
        total_read+=actual_read;
        length-=actual_read;
    }

    return total_read;
}

void ChainBuffer::append(ChainBuffer &&buf)
{
    for(buf_slice_iterator it=buf.slice_begin();it!=buf.slice_end();++it){
        this->_buf_slice_list.emplace_back(std::move(*it));
    }
    buf.clear();
}

void ChainBuffer::append(BufferSlice slice)
{
    this->_buf_slice_list.emplace_back(std::move(slice));
}

void ChainBuffer::prepend(BufferSlice slice)
{
    this->_buf_slice_list.emplace_front(std::move(slice));
}

void ChainBuffer::prepend(const std::string &data)
{
    size_t data_len=data.length();
    auto buf_ptr=BufferRaw::create<BufferMalloc>(data_len);
    std::memcpy(buf_ptr->data(),data.data(),data_len);
    BufferSlice slice(buf_ptr,0,data_len);
    this->prepend(slice);
}

std::unique_ptr<ChainBuffer> ChainBuffer::create_chain_buffer()
{
    return std::make_unique<ChainBuffer>();
}

void ChainBuffer::clear()
{
    _buf_slice_list.clear();
}

void ChainBuffer::commit_consume(size_t n_bytes)
{
    auto it_head=_buf_slice_list.begin();
    while(n_bytes>0&&it_head!=_buf_slice_list.end()){
        BufferSlice &buf_slice=(*it_head);
        if(buf_slice.readable_size()>n_bytes){
            buf_slice.advance_read(n_bytes);
            n_bytes=0;
        }
        else{
            size_t read_space=buf_slice.readable_size();
            buf_slice.advance_read(read_space);
            _buf_slice_list.pop_front();
            it_head=_buf_slice_list.begin();
            n_bytes-=read_space;
        }
    }
}

ChainBuffer ChainBuffer::slice(read_iterator begin,read_iterator end)
{
    ChainBuffer slice_buf;
    size_t length=end-begin;
    while(length){
        auto sptr=begin._it_buf_slice->raw_buf_sptr();
        size_t slice_len=std::min(static_cast<size_t>(begin._p_end-begin._p_cur),length);
        size_t start_pos=static_cast<size_t>(begin._p_cur-sptr->data());
        BufferSlice slice(sptr,start_pos,start_pos+slice_len);
        slice_buf.append(slice);
        length-=slice_len;
        begin+=slice_len;
    }
    return slice_buf;
}


void ChainBuffer::check_need_expand()
{
    if(!buf_slice_num()||_buf_slice_list.back().writable_size()==0){
        BufferRaw::Ptr fixed_buf=BufferRaw::create<BufferFixed<>>();
        BufferSlice slice(fixed_buf);
        _buf_slice_list.emplace_back(std::move(slice));
    }
}

ChainBuffer &ChainBuffer::operator<<(const std::string &arg)
{
    append(arg.c_str(),arg.length());
    return *this;
}

ChainBuffer &ChainBuffer::operator<<(const char c)
{
    append(&c,sizeof(c));
    return *this;
}

ChainBuffer::read_iterator ChainBuffer::begin()
{
    if(_buf_slice_list.empty()){
        return end();
    }
    return read_iterator(this,_buf_slice_list.begin(),_buf_slice_list.front().read_pos());
}

ChainBuffer::read_iterator ChainBuffer::end()
{
    return read_iterator(this,_buf_slice_list.end(),nullptr);
}

size_t ChainBuffer::buf_slice_num() const
{
    return _buf_slice_list.size();
}

size_t ChainBuffer::readable_size()
{
    return end()-begin();
}

bool ChainBuffer::empty()
{
    return this->_buf_slice_list.empty();
}

ChainBuffer::buf_slice_iterator ChainBuffer::slice_begin()
{
    return _buf_slice_list.begin();
}

ChainBuffer::buf_slice_iterator ChainBuffer::slice_end()
{
    return _buf_slice_list.end();
}

std::vector<std::pair<uint8_t*,size_t>> ChainBuffer::get_data()
{
    std::vector<std::pair<uint8_t *,size_t>> buf_data;
    for(auto slice_it=this->slice_begin();slice_it!=this->slice_end();++slice_it){
        buf_data.emplace_back(slice_it->read_pos(),slice_it->readable_size());
    }
    return buf_data;
}

}//namespace Rinx
