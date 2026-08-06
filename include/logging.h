#ifndef LOGGING_INCLUDED
#define LOGGING_INCLUDED


#include <vector>
#include <mutex>
#include <string>
#include <string_view>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
// #include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>

#include "tostring.h"
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


} // namespace


namespace logrr {

    
using LogField = std::pair<std::string, std::string>;

template <typename T>
LogField field(const std::string& key, T&& value) 
{
    return LogField{std::move(key), tostr::convertToString(std::forward<T>(value))};
}

struct LogRecord 
{
    logrr::log_status status;
    std::string_view detail_status="";
    std::string_view message="";
    std::string_view file="";
    int line=0;
    std::string timepoint = timeToString(std::chrono::system_clock::now());
    std::vector<LogField> details = {};
    bool importance = true; 
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


class Logger 
{
public:
    Logger() = default;
    Logger(const Logger&) noexcept;
    Logger(Logger&&) noexcept;
    virtual ~Logger() = default;
    Logger& operator=(const Logger&) noexcept;
    Logger& operator=(Logger&&) noexcept;

    bool addSink(std::shared_ptr<ILogSink> sink) noexcept;
    bool log(const LogRecord& record) noexcept;

    bool lInfo(const std::string& where) noexcept;
    bool lInfo(const std::string& where, std::vector<LogField>&& dtls) noexcept;

    bool lCalled(const std::string& where) noexcept;
    bool lCalled(const std::string& where, std::vector<LogField>&& dtls) noexcept;

    bool lExeced(const std::string& where) noexcept;
    bool lExeced(const std::string& where, std::vector<LogField>&& dtls) noexcept;

    bool lFailed(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    bool lFailed(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept;
    bool lFailed(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept;
    bool lFailed(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;

    bool lError(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    bool lError(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept;
    bool lError(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept;
    bool lError(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;

    bool lWarning(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    bool lWarning(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept;
    bool lWarning(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept;
    bool lWarning(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;

    bool lCritical(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;
    bool lDebug(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;
    bool lTrace(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;
    bool flush() noexcept;
protected:
    bool standardTempl(
        logrr::log_status status,
        const std::string& where, 
        std::vector<LogField>&& dtls={}, 
        const std::string& sentence_prefix=""
    ) noexcept;

    bool detailedTempl(
        logrr::log_status status,
        const std::string& where, 
        std::vector<LogField>&& dtls={}, 
        const std::string& sentence_prefix="",
        const std::string& file="", 
        int line=0
    ) noexcept;
protected:
    std::mutex mtx_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};


inline bool Logger::standardTempl(
    logrr::log_status status,
    const std::string& where, 
    std::vector<LogField>&& dtls, 
    const std::string& sentence_prefix
) noexcept {
return log(LogRecord{
    .status = status,
    .message = where + sentence_prefix,
    .details = std::forward<std::vector<LogField>>(dtls),
    .importance = false,
    });
}

inline bool Logger::detailedTempl(
    logrr::log_status status,
    const std::string& where, 
    std::vector<LogField>&& dtls, 
    const std::string& sentence_prefix,
    const std::string& file, 
    int line
) noexcept {
return log(LogRecord{
    .status = status,
    .message = where + sentence_prefix,
    .file = file,
    .line = line,
    .details = std::forward<std::vector<LogField>>(dtls),
    });
}


/// lInfo
inline bool Logger::lInfo(const std::string& where) noexcept 
{
    return standardTempl(logrr::log_status::info, where);
}

inline bool Logger::lInfo(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls));
}


/// lCalled
inline bool Logger::lCalled(const std::string& where) noexcept 
{
    return standardTempl(logrr::log_status::info, where, {}, " was called");
}

inline bool Logger::lCalled(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), " was called");
}


/// lExeced
inline bool Logger::lExeced(const std::string& where) noexcept 
{
    return standardTempl(logrr::log_status::info, where, {}, " executed successfully");
}

inline bool Logger::lExeced(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), " executed successfully");
}


/// lFailed
inline bool Logger::lFailed(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), " failed");
}

inline bool Logger::lFailed(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), " failed", "", line);
}

inline bool Logger::lFailed(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), " failed", file);
}

inline bool Logger::lFailed(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), " failed", file, line);
}


/// lError
inline bool Logger::lError(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls));
}

inline bool Logger::lError(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", "", line);
}

inline bool Logger::lError(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", file);
}

inline bool Logger::lError(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


/// lWarning
inline bool Logger::lWarning(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::warning, where, std::forward<std::vector<LogField>>(dtls));
}

inline bool Logger::lWarning(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::warning, where, std::forward<std::vector<LogField>>(dtls), "", "", line);
}

inline bool Logger::lWarning(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept 
{
    return detailedTempl(logrr::log_status::warning, where, std::forward<std::vector<LogField>>(dtls), "", file);
}

inline bool Logger::lWarning(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailedTempl(logrr::log_status::warning, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


/// lCritical
inline bool Logger::lCritical(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailedTempl(logrr::log_status::critical, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


/// lDebug
inline bool Logger::lDebug(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailedTempl(logrr::log_status::debug, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


/// lTrace
inline bool Logger::lTrace(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailedTempl(logrr::log_status::trace, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}

} // namespace logrr

#endif