#ifndef CONCAT_INCLUDED
#define CONCAT_INCLUDED

#include <sstream>
#include <type_traits>

template <typename, typename=void>
struct isStreamableImpl : std::false_type {};

template <typename T> 
struct isStreamableImpl<
    T,
    std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<T>())>
> : std::true_type {};

template <typename T>
inline constexpr bool isStreamable = isStreamableImpl<T>::value;


template <typename... Args>
std::string concat(Args&&... args) {
    static_assert((isStreamable<Args> && ...), 
        "concat requires all arguments to support operator<<");
    std::ostringstream oss;
    (oss << ... << args);
    return oss.str();
}

#endif