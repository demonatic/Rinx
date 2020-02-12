#ifndef UTIL_H
#define UTIL_H

#include <tuple>
#include <memory>
#include <functional>
#include <type_traits>
#include <iomanip>
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

/// @brief convert enum class to its underlying arithmetic type
//TODO template <typename E,std::enable_if_t<std::is_arithmetic_v<typename std::enable_if_t<std::is_enum_v<E>,std::underlying_type_t<E>>>>>
template <typename E>
constexpr auto to_index(E e) noexcept
{    
    return static_cast<std::underlying_type_t<E>>(e);
}

inline bool hex_str_to_size_t(const std::string &hex_string, size_t &hex)
{
    try {
        hex=std::stoull(hex_string,nullptr,16);
    } catch (std::invalid_argument &) {
        return false;
    }
    return true;
}

template<typename T>
inline std::string int_to_hex(T i)
{
   std::stringstream stream;
   stream << std::hex << i;
   return stream.str();
}

inline std::optional<size_t> str_to_size_t(const std::string &str){
    std::optional<size_t> val;
    try {
        val.emplace(std::stoull(str));
    } catch (std::exception &) {
        val=std::nullopt;
    }
    return val;
}

}
#endif // UTIL_H
