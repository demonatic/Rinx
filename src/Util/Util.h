#ifndef UTIL_H
#define UTIL_H

#include <tuple>
#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <iostream>

#if !defined(__GNUC__)
#define __buildin_expect(x,expect_val) (x)
#endif

#ifndef likely
#define likely(x) __builtin_expect(!!(x),1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x),0)
#endif

namespace Util {

template<typename T,typename Tuple>
struct GetTupleIndex;

template<typename T,typename... Types>
struct GetTupleIndex<T,std::tuple<T,Types...>>{
    static constexpr const::size_t value=0;
};

template<typename T,typename U,typename... Types>
struct GetTupleIndex<T,std::tuple<U,Types...>>{
    static constexpr const::size_t value=1+GetTupleIndex<T,std::tuple<Types...>>::value;
};

template <size_t I>
struct visit_impl
{
    template <typename T, typename F>
    static void visit(T& tup, size_t idx, F fun)
    {
        if (idx == I - 1) fun(std::get<I - 1>(tup).get());
        else visit_impl<I - 1>::visit(tup, idx, fun);
    }
};

template <>
struct visit_impl<0>
{
    template <typename T, typename F>
    static void visit([[maybe_unused]]T& tup,[[maybe_unused]] size_t idx,[[maybe_unused]] F fun)
    { assert(false); }
};

template <typename F, typename... Ts>
void visit_at(std::tuple<Ts...> const& tup, size_t idx, F fun)
{
    visit_impl<sizeof...(Ts)>::visit(tup, idx, fun);
}

//visit a tuple of std::unique_ptr's index element in runtime
template <typename F, typename... Ts>
void get_tuple_at(std::tuple<Ts...>& tup, size_t idx, F fun)
{
    visit_impl<sizeof...(Ts)>::visit(tup, idx, fun);
}

//visit each of the element in a tuple and apply it with func f
template<std::size_t I = 0, typename F,typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
  tuple_for_each(std::tuple<Tp...> &,[[maybe_unused]] F &&f) // Unused arguments are given no names.
{}

template<std::size_t I = 0,typename F,typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
tuple_for_each(std::tuple<Tp...>& t,F &&f)
{
    f(std::get<I>(t));
    tuple_for_each<I + 1, F, Tp...>(t,f);
}

inline void to_lower(std::string &str) noexcept{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

inline void to_upper(std::string &str) noexcept{
    std::transform(str.begin(),str.end(),str.begin(),::toupper);
}

template <typename E>
constexpr auto to_underlying_type(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

template<typename T>
struct TypeErasedIterator:std::iterator<
        std::input_iterator_tag,T,
        std::ptrdiff_t,
        T*,
        T& >
{
    struct IErase{
        virtual T& get_elm()=0;
        virtual void advance()=0;
        virtual bool equal(const IErase &it) const=0;
        virtual IErase* clone() const=0;
    };

    template<class It>
    struct EraseImpl:IErase{
        EraseImpl(It &&it):_it(std::forward<It>(it)){}
        EraseImpl(const EraseImpl&)=default;
        It _it;
        virtual T& get_elm() override{ return *_it; }
        virtual void advance() override{ ++_it; }
        virtual bool equal(const IErase &other) const override{ return static_cast<EraseImpl const&>(other)._it==_it; }
        virtual IErase* clone() const override{ return new EraseImpl(*this);}
    };

    template<typename It,
             class=std::enable_if<
                std::is_convertible<typename std::iterator_traits<It>::reference,T>{}>>
    TypeErasedIterator(It &&it):_erased_it(std::make_unique<EraseImpl<It>>(std::forward<It>(it))) {}

    TypeErasedIterator(TypeErasedIterator &&other)=default;
    TypeErasedIterator()=default;

    bool operator==(const TypeErasedIterator &other){
        return _erased_it->equal(*other._erased_it);
    }

    bool operator!=(const TypeErasedIterator &other){
        return !(*this==other);
    }

    TypeErasedIterator(TypeErasedIterator const& other){
        this->_erased_it.reset(other._erased_it?other._erased_it->clone():nullptr);
    }


    TypeErasedIterator& operator++(){
        _erased_it->advance();
        return *this;
    }

    TypeErasedIterator operator++(int){
        auto tmp=*this;
        ++(*this);
        return tmp;
    }

    T& operator*() const{
        return _erased_it->get_elm();
    }

private:
    std::unique_ptr<IErase> _erased_it;
};

template<typename T>
class ErasedDeleter:std::function<void(T*)>{
public:
    ErasedDeleter():std::function<void(T*)>(
        [](T* t){delete t;}
    ){}
};

template<typename T>
using ErasedUptr=std::unique_ptr<T,ErasedDeleter<T>>;

using unique_void_ptr = std::unique_ptr<void, void(*)(void const*)>;

template<typename T>
auto unique_void(T * ptr) -> unique_void_ptr
{
    return unique_void_ptr(ptr, [](void const * data) {
         T const * p = static_cast<T const*>(data);
         std::cout << "{" << *p << "} located at [" << p <<  "] is being deleted.\n";
         delete p;
    });
}

}
#endif // UTIL_H
