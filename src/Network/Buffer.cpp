#include "Buffer.h"

size_t ChainBuffer::buf_slice_num() const
{
    return _buf_slice_list.size();
}

size_t ChainBuffer::readable_size()
{
    return readable_end()-readable_begin();
}

ssize_t ChainBuffer::read_from_fd(int fd,RxReadRc &res)
{
    this->check_need_expand();

    char extra_buff[65535];
    std::vector<struct iovec> io_vecs(2);
    BufferSlice &tail=get_tail();
    io_vecs[0].iov_base=tail.write_pos();
    io_vecs[0].iov_len=tail.writable_size();
    io_vecs[1].iov_base=extra_buff;
    io_vecs[1].iov_len=sizeof extra_buff;

    ssize_t read_bytes=RxFDHelper::Stream::readv(fd,io_vecs,res);
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

ssize_t ChainBuffer::write_to_fd(int fd,RxWriteRc &res)
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

    ssize_t bytes=RxFDHelper::Stream::writev(fd,io_vecs,res);
    if(bytes>0){
        this->commit_consume(bytes);
    }
    return bytes;
}

bool ChainBuffer::append(int regular_file_fd,size_t length,size_t offset)
{
    try{
        auto buf_file=BufferBase::create<BufferFile>(regular_file_fd,length);
        BufferSlice file(buf_file,offset,buf_file->length());
        this->push_buf_slice(file);
    }
    catch(std::bad_alloc &){
        return false;
    }
    return true;
}

ChainBuffer::read_iterator ChainBuffer::readable_begin()
{
    return read_iterator(this,_buf_slice_list.begin(),_buf_slice_list.front().read_pos());
}

ChainBuffer::read_iterator ChainBuffer::readable_end()
{
    return read_iterator(this,_buf_slice_list.end(),nullptr);
}

ChainBuffer::buf_slice_iterator ChainBuffer::slice_begin()
{
    return _buf_slice_list.begin();
}

ChainBuffer::buf_slice_iterator ChainBuffer::slice_end()
{
    return _buf_slice_list.end();
}

void ChainBuffer::append(const char *data, size_t length)
{
    const char *pos=data;
    size_t bytes_left=length;

    while(bytes_left>0){
        this->check_need_expand();

        BufferSlice &tail=this->get_tail();
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
    for(buf_slice_iterator it=buf.slice_begin();it!=buf.slice_end();++it){
        this->_buf_slice_list.emplace_back(std::move((*it)));
    }
    buf.free();
}


BufferSlice& ChainBuffer::get_head()
{
    return _buf_slice_list.front();
}

BufferSlice& ChainBuffer::get_tail()
{
    return _buf_slice_list.back();
}

std::unique_ptr<ChainBuffer> ChainBuffer::create_chain_buffer()
{
    return std::make_unique<ChainBuffer>();
}

void ChainBuffer::free()
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
            //TODO add low chunk num thresh
            if(this->pop_unused_buf_slice()){
                it_head=_buf_slice_list.begin();
            }
            n_bytes-=read_space;
        }
    }
}

ChainBuffer ChainBuffer::slice(read_iterator begin,read_iterator end)
{
    ChainBuffer sliced;
    size_t length=end-begin;
    while(length){
        size_t slice_len=std::min(static_cast<size_t>(begin._p_end-begin._p_cur),length);
        size_t start_pos=static_cast<size_t>(begin._p_cur-begin._p_start);
        sliced.push_buf_slice(BufferSlice(begin._it_buf_slice->get_buf_raw_ptr(),start_pos,start_pos+slice_len));
        length-=slice_len;
        begin+=slice_len;
    }
    return sliced;
}

void ChainBuffer::push_buf_slice(BufferSlice slice)
{
    _buf_slice_list.emplace_back(std::move(slice));
}

bool ChainBuffer::pop_unused_buf_slice(bool force)
{
    if(this->buf_slice_num()&&this->get_head().readable_size()!=0&&!force){
        return false;
    }
    _buf_slice_list.pop_front();
    return true;
}

void ChainBuffer::check_need_expand()
{
    if(!buf_slice_num()||get_tail().writable_size()==0){
        BufferSlice slice(BufferBase::create<BufferFixed<>>());
        push_buf_slice(slice);
    }
}

ChainBuffer &ChainBuffer::operator<<(const std::string &arg)
{
    append(arg.c_str(),arg.length());
    return *this;
}

