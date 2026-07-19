#ifndef LOGGING_H
#define LOGGING_H


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


struct LogRecord {
    LogLevel level;
    std::string message="none";
    std::string file="none";
    int line=0;
    std::string func="none";
    std::string timepoint=timeToString(std::chrono::system_clock::now());
};

struct ILogFormatter {
    virtual ~ILogFormatter() = default;
    virtual std::string format(const LogRecord& record) const noexcept = 0;
};


class SingleLineFormatter final : public ILogFormatter {
public:
    SingleLineFormatter(){}
    ~SingleLineFormatter() = default;
    std::string format(const LogRecord& r) const noexcept override;
private:
    std::string formatCompact(const LogRecord& r) const noexcept;
    std::string formatDetailed(const LogRecord& r) const noexcept;
};


struct JsonFormatter final : public ILogFormatter {
public:
    JsonFormatter(){}
    ~JsonFormatter() = default;
    std::string format(const LogRecord& r) const noexcept override;
private:
    std::string formatCompact(const LogRecord& r) const noexcept;
    std::string formatDetailed(const LogRecord& r) const noexcept;
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
    FileSink(const std::string& file_name, std::shared_ptr<ILogFormatter> formatter);
    FileSink(const std::string& file_name) : FileSink(file_name, std::make_shared<SingleLineFormatter>()) {}
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
    Logger(){}
    bool addSink(std::shared_ptr<ILogSink> sink) noexcept;
    bool log(const LogRecord& record) noexcept;
    bool log(const LogLevel& level, const std::string& message) noexcept;
    bool log(const LogLevel& level, const std::string& message, const std::string& file) noexcept;
    bool log(const LogLevel& level, const std::string& message, 
             const std::string& file, int line) noexcept;
    bool log(const LogLevel& level, const std::string& message, 
             const std::string& file, int line, const std::string& func) noexcept;
    bool flush() noexcept;
private:
    std::mutex mtx_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

} // namespace logrr

#endif