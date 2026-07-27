#ifndef LOGGING_H
#define LOGGING_H


#include <variant>
#include <vector>
#include <mutex>
#include <string>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <ctime>
#include <iomanip>

#include "concat.h"

namespace {
    std::string timeToString(std::chrono::system_clock::time_point tp) {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%F %T");
        return oss.str();
    }
} // namespace


namespace logrr {

class LogLevel final {
public:
    LogLevel() : num_(LogLevel::Info.num()), name_(LogLevel::Info.name()) {}
    constexpr LogLevel(int num, const char* name) : num_(num), name_(name) {}

    const char* name() const { return name_; }
    int num() const { return num_; }

    static const LogLevel Critical;
    static const LogLevel Error;
    static const LogLevel Warning; 
    static const LogLevel Info;
    static const LogLevel Debug;
    static const LogLevel Trace;

private:
    int num_;
    const char* name_;
};

inline constexpr LogLevel LogLevel::Critical{-2, "CRIT"};
inline constexpr LogLevel LogLevel::Error{-1, "ERROR"};
inline constexpr LogLevel LogLevel::Warning{0, "WARN"};
inline constexpr LogLevel LogLevel::Info{1, "INFO"};
inline constexpr LogLevel LogLevel::Debug{2, "DEBUG"};
inline constexpr LogLevel LogLevel::Trace{3, "TRACE"};

enum class StatusCode : uint16_t {
    OK = 200,
    ACCEPTED = 201,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    NOT_FOUND = 404,
    INTERNAL_ERROR = 500,
};

constexpr const char* codeToStr(StatusCode code) {
    switch (code) {
        case StatusCode::OK:             return "OK";
        case StatusCode::ACCEPTED:       return "Accepted";
        case StatusCode::BAD_REQUEST:    return "Bad Request";
        case StatusCode::NOT_FOUND:      return "Not Found";
        case StatusCode::INTERNAL_ERROR: return "Internal HttpServer Error";
        default:                         return "Unknown";
    }
}

using LogValue = std::variant<std::string, uint16_t, int64_t, double, bool, StatusCode>;

struct LogField {
    std::string key;
    LogValue value;
};

struct LogRecord {
    LogLevel level;
    std::string_view message="";
    std::string_view file="";
    int line=0;
    std::string_view func="";
    std::string timepoint = timeToString(std::chrono::system_clock::now());
    std::vector<LogField> fields = {};
};

struct ILogFormatter {
    virtual ~ILogFormatter() = default;
    virtual std::string format(const LogRecord& record) const noexcept = 0;
};


class SingleLineFormatter : public ILogFormatter {
public:
    SingleLineFormatter() = default;
    virtual ~SingleLineFormatter() = default;
    std::string format(const LogRecord& r) const noexcept override;
};


struct JsonFormatter : public ILogFormatter {
public:
    JsonFormatter() = default;
    virtual ~JsonFormatter() = default;
    std::string format(const LogRecord& r) const noexcept override;
};


struct ILogSink {
    virtual ~ILogSink() = default;
    virtual bool log(const LogRecord& record) noexcept = 0;
    virtual bool flush() noexcept { return true; }
    virtual const char* name() const noexcept = 0;
};


class ConsoleSink final : public ILogSink {
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


class FileSink final : public ILogSink {
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

class Logger final {
public:
    Logger() = default;
    Logger(const Logger& logger) noexcept;
    Logger(Logger&& logger) noexcept;
    ~Logger() = default;
    Logger& operator=(const Logger& logger) noexcept;
    Logger& operator=(Logger&& logger) noexcept;

    bool addSink(std::shared_ptr<ILogSink> sink) noexcept;
    bool log(const LogRecord& record) noexcept;
    bool log(const LogLevel& level, std::string_view message) noexcept;
    bool log(const LogLevel& level, std::string_view message, std::string_view file) noexcept;
    bool log(const LogLevel& level, std::string_view message, 
             std::string_view file, int line) noexcept;
    bool log(const LogLevel& level, std::string_view message, 
             std::string_view file, int line, std::string_view func) noexcept;
    bool flush() noexcept;
private:
    std::mutex mtx_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

} // namespace logrr

#endif