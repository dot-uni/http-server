#ifndef JSON_FORMATTER
#define JSON_FORMATTER

#include <fmt/format.h>
#include <type_traits>
#include <string_view>

namespace {


template <typename, typename=void>
struct is_range : std::false_type {};

template <typename T>
struct is_range<
    T,
    std::void_t<
        decltype(std::begin(std::declval<T&>())),
        decltype(std::end(std::declval<T&>()))
    >
> : std::true_type {};

template <typename T>
constexpr bool is_range_v = 
    is_range<T>::value &&
    !std::is_same_v<std::decay_t<T>, std::string> &&
    !std::is_convertible_v<std::decay_t<T>, const char*>;


template <typename T>
struct is_str : std::is_same<std::string, typename std::decay<T>::type> {};

template <typename T>
struct is_cstring : std::is_same<const char*, typename std::decay<T>::type> {};

template <typename T>
struct is_char : std::is_same<char, typename std::decay<T>::type> {};

template <typename T>
struct is_string_view : std::is_same<std::string_view, typename std::decay<T>::type> {};

template <typename T>
constexpr bool is_str_v = is_str<T>::value;

template <typename T>
constexpr bool is_string_v = 
    is_str<T>::value ||
    is_cstring<T>::value ||
    is_char<T>::value ||
    is_string_view<T>::value
;


auto is_valid_str = [](auto arg) -> bool {
    using type = std::decay_t<decltype(arg)>;
    if constexpr (is_str_v<type>) {
        if (arg.empty()) return false;
        if (
            (arg.front() == '{' && arg.back() == '}') ||
            (arg.front() == '[' && arg.back() == ']')
        ) { return true; }
        int quote = arg.find("\"", 1);
        int colon = arg.find(":");
        if (
            arg.front() == '"' && 
            quote != std::string::npos && 
            colon != std::string::npos &&
            colon - quote == 1
        ) { return true; }
        return false;
    }
    return false;
};


} // namespace


namespace json {


struct none {};


auto check_type = [](const auto& arg) -> std::string {
    using type = std::decay_t<decltype(arg)>;
    if (is_valid_str(arg)) {
        return fmt::format(R"({})", arg);
    }
    else if constexpr (is_string_v<type>) {
        return fmt::format(R"("{}")", arg);
    }
    return fmt::format(R"({})", arg);
};


auto field = [](const auto& fst, const auto& sec) -> std::string {
    using type = std::decay_t<decltype(sec)>;
    if (is_valid_str(sec)) {
        return fmt::format(R"("{}":{})", fst, sec);
    }
    else if constexpr (is_string_v<type>) {
        return fmt::format(R"("{}":"{}")", fst, sec);
    }
    else if constexpr (std::is_same<type, none>::value) {
        return fmt::format(R"("{}":none)", fst);
    }
    return fmt::format(R"("{}":{})", fst, sec);
};


template <typename... Args>
std::string obj(Args&&... args) {
    std::string res = "{";
    if constexpr (sizeof...(args) == 0) return "{}";
    else if constexpr (sizeof...(args) == 1) {
        using Arg0 = std::tuple_element_t<0, std::tuple<std::decay_t<Args>...>>;

        if constexpr (is_range_v<Arg0>) {
            auto&& single = std::get<0>(std::forward_as_tuple(args...));
            if (single.empty()) return "{}";
            for (auto&& [key, value] : single) {
                res += field(key, value) + ",";
            }
        }
        else {
            ((res += check_type(args) + ","), ...);
        }
    }
    else {
        ((res += check_type(args) + ","), ...);
    }
    res.back() = '}';
    return res;
}


template <typename... Args>
std::string arr(Args&&... args) {
    std::string res = "[";
    if constexpr (sizeof...(args) == 0) return "[]";
    else if constexpr (sizeof...(args) == 1) {
        using Arg0 = std::tuple_element_t<0, std::tuple<std::decay_t<Args>...>>;

        if constexpr (is_range_v<Arg0>) {
            auto&& single = std::get<0>(std::forward_as_tuple(args...));
            if (single.empty()) return "[]";
            for (auto&& [key, value] : single) {
                res += field(key, value) + ",";
            }
        }
        else {
            ((res += check_type(args) + ","), ...);
        }
    }
    else {
        ((res += check_type(args) + ","), ...);
    }
    res.back() = ']';
    return res;
}

} // namespace json  

#endif 