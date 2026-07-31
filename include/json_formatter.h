#ifndef JSON_FORMATTER
#define JSON_FORMATTER

#include <fmt/format.h>
#include <type_traits>
#include <string_view>

namespace {

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

template <typename Func>
struct repeat_t 
{
    Func foo;
    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const {
        return foo(*this, std::forward<Args>(args)...);
    }
};

template <typename Func> 
repeat_t<std::decay_t<Func>> repeat(Func&& foo) {
    return { std::forward<Func>(foo) };
}

auto check_type = [](auto arg) -> std::string {
    using type = std::decay_t<decltype(arg)>;
    if (is_valid_str(arg)) {
        return fmt::format(R"({})", arg);
    }
    else if constexpr (is_string_v<type>) {
        return fmt::format(R"("{}")", arg);
    }
    return fmt::format(R"({})", arg);
};

auto field = [](auto fst, auto sec) -> std::string {
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
    // bool, int, double, float
    return fmt::format(R"("{}":{})", fst, sec);
};

auto var = [](auto arg) -> std::string { 
    return check_type(arg);
};

auto obj = repeat([](auto&& self, auto... args) -> std::string
{
    std::string res = "{";
    if constexpr (sizeof...(args) == 0) return "{}";
    ((res += check_type(args) + ","), ...);
    res.back() = '}';
    return res;
});

auto arr = repeat([](auto&& self, auto... args) -> std::string
{
    std::string res = "[";
    if constexpr (sizeof...(args) == 0) return "[]";
    ((res += check_type(args) + ","), ...);
    res.back() = ']';
    return res;
});


} // namespace json  

#endif 