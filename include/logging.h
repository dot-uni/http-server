#ifndef LOGGING_H
#define LOGGING_H


#include <vector>
#include <mutex>
#include <string>
#include <string_view>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <ctime>
#include <iomanip>
#include <type_traits>

#include "json_formatter.h"
#include "log_status.h"


namespace {


std::string timeToString(std::chrono::system_clock::time_point tp) 
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%F %T");
    return oss.str();
}


template <typename, typename=void>
struct is_formattable_to_string : std::false_type {};

template <typename T>
struct is_formattable_to_string<
    T,
    std::void_t<decltype(static_cast<std::string>(std::declval<T>()))>
> : std::true_type {};

template <typename T>
constexpr bool is_formattable_to_string_v = is_formattable_to_string<T>::value;


template <typename, typename=void>
struct is_streamable : std::false_type {};

template <typename T> 
struct is_streamable<
    T,
    std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<T>())>
> : std::true_type {};

template <typename T>
constexpr bool is_streamable_v = is_streamable<T>::value;


template <typename T>
std::string convertToString(T&& value) {
    using U = std::decay_t<T>;

    if constexpr (is_formattable_to_string_v<U>) {
        return static_cast<std::string>(std::forward<T>(value));
    } 
    else if constexpr (std::is_enum_v<U>) {
        return std::to_string(static_cast<std::underlying_type_t<U>>(value));
    }
    else if constexpr (std::is_same<U, bool>::value) {
        return value ? "true" : "false";
    } 
    else if constexpr (std::is_arithmetic<U>::value) {
        return std::to_string(value);
    } 
    else if constexpr (is_streamable_v<U>) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    } 
    else {
        static_assert(!sizeof(T*), "Type has no known conversion to std::string");
    }
}


} // namespace


namespace logrr {

    
using LogField = std::pair<std::string, std::string>;


template <typename T>
LogField field(const std::string& key, T&& value) 
{
    return LogField{std::move(key), convertToString(std::forward<T>(value))};
}


struct LogRecord 
{
    log_status status;
    std::string_view message="";
    std::string_view file="";
    int line=0;
    std::string timepoint = timeToString(std::chrono::system_clock::now());
    std::vector<LogField> details = {};
};


struct ILogFormatter 
{
    virtual ~ILogFormatter() = default;
    virtual std::string format(const LogRecord&) const noexcept = 0;
};


class SingleLineFormatter : public ILogFormatter 
{
public:
    SingleLineFormatter() = default;
    virtual ~SingleLineFormatter() = default;
    std::string format(const LogRecord& r) const noexcept override;
};


struct JsonFormatter : public ILogFormatter 
{
public:
    JsonFormatter() = default;
    virtual ~JsonFormatter() = default;
    std::string format(const LogRecord& r) const noexcept override;
};


struct ILogSink 
{
    virtual ~ILogSink() = default;
    virtual bool log(const LogRecord& record) noexcept = 0;
    virtual bool flush() noexcept { return true; }
    virtual const char* name() const noexcept = 0;
};


class ConsoleSink final : public ILogSink 
{
public:
    ConsoleSink();
    ConsoleSink(std::shared_ptr<ILogFormatter> formatter);
    ~ConsoleSink() = default;
    bool log(const LogRecord& record) noexcept override;
    const char* name() const noexcept override { return "ConsoleSink"; }
private:
    std::mutex mtx_;
    std::shared_ptr<ILogFormatter> formatter_;
};


class FileSink final : public ILogSink 
{
public:
    FileSink();
    FileSink(const std::string& file_name);
    FileSink(const std::string& file_name, std::shared_ptr<ILogFormatter> formatter);
    ~FileSink() { file_.close(); }
    bool log(const LogRecord& record) noexcept override;
    bool flush() noexcept override;
    const char* name() const noexcept override { return "FileSink"; }
private:
    std::mutex mtx_;
    std::ofstream file_;
    std::shared_ptr<ILogFormatter> formatter_;
};


class Logger final 
{
public:
    Logger() = default;
    Logger(const Logger& logger) noexcept;
    Logger(Logger&& logger) noexcept;
    ~Logger() = default;
    Logger& operator=(const Logger& logger) noexcept;
    Logger& operator=(Logger&& logger) noexcept;

    bool addSink(std::shared_ptr<ILogSink> sink) noexcept;
    bool log(const LogRecord& record) noexcept;

    inline bool linfo(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    inline bool lcalled(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    inline bool lexeced(const std::string& where, std::vector<LogField>&& dtls) noexcept;

    inline bool lfailed(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    inline bool lfailed(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept;
    inline bool lfailed(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept;
    inline bool lfailed(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;

    inline bool lerror(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    inline bool lerror(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept;
    inline bool lerror(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept;
    inline bool lerror(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;

    inline bool lwarning(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    inline bool lwarning(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept;
    inline bool lwarning(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept;
    inline bool lwarning(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;

    inline bool lcritical(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;
    inline bool ldebug(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;
    inline bool ltrace(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;
    bool flush() noexcept;
protected:
    inline bool simple_templ(
        log_status status,
        const std::string& where, 
        std::vector<LogField>&& dtls, 
        const std::string& sentence_prefix=""
    ) noexcept;

    inline bool detailed_templ(
        log_status status,
        const std::string& where, 
        std::vector<LogField>&& dtls, 
        const std::string& sentence_prefix="",
        const std::string& file="", 
        int line=0
    ) noexcept;
private:
    std::mutex mtx_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

} // namespace logrr

#endif