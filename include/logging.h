#ifndef LOGGING_H
#define LOGGING_H


#include <variant>
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

#include "concat.h"
#include "client_structures.h"
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
    struct isConvertableToString : std::false_type {};
    
    template <typename T>
    struct isConvertableToString<
        T,
        std::void_t<decltype(static_cast<std::string>(std::declval<T>()))>
    > : std::true_type {};


    template <typename, typename=void>
    struct isStreamable : std::false_type {};

    template <typename T> 
    struct isStreamable<
        T,
        std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<T>())>
    > : std::true_type {};
    

    template <typename T>
    std::string convertToString(T&& value) 
    {
        using U = std::decay_t<T>;

        if constexpr (isConvertableToString<U>::value) {
            return static_cast<std::string>(std::forward<T>(value));
        } 
        else if constexpr (std::is_enum_v<U>) {
            return std::to_string(static_cast<std::underlying_type_t<U>>(value));
        }
        else if constexpr (std::is_same_v<U, bool>) {
            return value ? "true" : "false";
        } 
        else if constexpr (std::is_arithmetic_v<U>) {
            return std::to_string(value);
        } 
        else if constexpr (isStreamable<U>::value) {
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

struct LogField 
{
    std::string key;
    std::string value;
};

template <typename T>
LogField field(const std::string& key, T&& value) 
{
    return LogField{std::move(key), convertToString(std::forward<T>(value))};
}

struct LogRecord 
{
    log_status level;
    std::string_view message="";
    std::string_view file="";
    int line=0;
    std::string_view func="";
    std::string timepoint = timeToString(std::chrono::system_clock::now());
    std::vector<LogField> fields = {};
};

struct ILogFormatter 
{
    virtual ~ILogFormatter() = default;
    virtual std::string format(const LogRecord& record) const noexcept = 0;
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
    ConsoleSink() : formatter_(std::make_shared<SingleLineFormatter>()) {}
    ConsoleSink(std::shared_ptr<ILogFormatter> formatter) : formatter_(std::move(formatter)) {}
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
    FileSink(std::string_view file_name, std::shared_ptr<ILogFormatter> formatter);
    FileSink(std::string_view file_name) : FileSink(file_name, std::make_shared<JsonFormatter>()) {}
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
    bool log(const log_status& level, std::string_view message) noexcept;
    bool log(const log_status& level, std::string_view message, std::string_view file) noexcept;
    bool log(const log_status& level, std::string_view message, 
             std::string_view file, int line) noexcept;
    bool log(const log_status& level, std::string_view message, 
             std::string_view file, int line, std::string_view func) noexcept;
    bool flush() noexcept;
private:
    std::mutex mtx_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

} // namespace logrr

#endif