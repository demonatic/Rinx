#ifndef OBJECTALLOCATOR_H
#define OBJECTALLOCATOR_H
#include <boost/pool/pool.hpp>
#include <memory>
#include <thread>
#include <iostream>
#include <atomic>

namespace RxAllocator {

template<typename T>
class ObjectAllocator
{
public:
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    auto static constexpr block_size=64+sizeof(value_type);

public:
    ObjectAllocator() noexcept{}
    ObjectAllocator(const ObjectAllocator &other) noexcept {}
    ~ObjectAllocator()=default;

    template<typename U>
    ObjectAllocator(const ObjectAllocator<U> &other) noexcept {}

    template<typename U>
    ObjectAllocator& operator= (const ObjectAllocator<U> &other){
        return *this;
    }

    ObjectAllocator<T>& operator = (const ObjectAllocator &other){
        return *this;
    }

    template<typename U>
    struct rebind{ typedef ObjectAllocator<U> other; };

    T *allocate(size_type n, const void *hint=nullptr){
#ifdef _DEBUG
        assert(n==1);
#endif
        T *addr=static_cast<T*>(pool_.malloc());
        if(!addr) throw std::bad_alloc();
        return addr;
    }

    void deallocate(T *ptr, size_type n){
#ifdef _DEBUG
        assert(n==1);
#endif
        if(ptr){
            pool_.free(ptr);
        }
    }

private:
    thread_local static boost::pool<> pool_;
};

template<typename T>
thread_local boost::pool<> ObjectAllocator<T>::pool_(block_size);


template<typename T, typename U>
inline bool operator == (const ObjectAllocator<T>&, const ObjectAllocator<U>&){
    return true;
}

template<typename T, typename U>
inline bool operator != (const ObjectAllocator<T>& a, const ObjectAllocator<U> &b){
    return !(a==b);
}


template <typename T>
thread_local static ObjectAllocator<T> allocator;
}

template <typename T, typename ...Args>
inline auto rx_pool_make_shared(Args&&... args){
    return std::allocate_shared<T,RxAllocator::ObjectAllocator<T>>(RxAllocator::allocator<T>,std::forward<Args>(args)...);
}


#endif // OBJECTALLOCATOR_H
