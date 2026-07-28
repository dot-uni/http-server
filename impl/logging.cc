#include "logging.h"


namespace logrr {

std::string SingleLineFormatter::format(const LogRecord& r) const noexcept 
{
    std::string base = "";
    try {
        base = fmt::format("{} [{}] – {}", 
            r.timepoint, logrr::obsolete_reason(r.level), r.message);
        if (!r.file.empty()) {
            base += fmt::format(R"(, "file": "{}")", r.file);
        }
        if (r.line) {
            base += fmt::format(R"(, "line": {})", r.line);
        }
        if (!r.func.empty()) {
            base += fmt::format(R"(, "func": {})", r.func);
        }
        for (auto&& field : r.fields) {
            base += fmt::format(R"(, "{}": {})", field.key, field.value);
        }
    }
    catch(fmt::format_error& mess) 
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " << mess.what() << '\n';
    }
    return base;
}

std::string JsonFormatter::format(const LogRecord& r) const noexcept 
{
    std::string base = "";
    try {
        base = fmt::format(R"({{"timepoint":"{}","level":"{}","message":"{}","file":"{}","line":"{}","func":"{}")", 
            r.timepoint, logrr::obsolete_reason(r.level), r.message, r.file, r.line, r.func);
        if (!r.fields.empty()) {
            base += R"("fields":{)";

            for (auto&& field : r.fields) {
                base += fmt::format(R"("{}":"{}",)", field.key, field.value);
            }
            base.back() = '}';
        }
    } catch(fmt::format_error& mess) {
        std::cerr << __FILE__ << ":" << __LINE__ << " " << mess.what() << '\n';
    }
    if (!base.empty()) base += "}";
    return base;
}

bool ConsoleSink::log(const LogRecord& record) noexcept 
{
    std::string inf;
    inf = formatter_->format(record);

    std::ostream& out = (record.level <= log_status::error) ? std::cerr : std::cout;
    out << inf << '\n';

    return static_cast<bool>(out);
}

FileSink::FileSink(std::string_view file_name, std::shared_ptr<ILogFormatter> formatter) 
{
    file_.open(file_name, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error(fmt::format("{}:{} Failed to open file '{}': {}", 
                                    __FILE__, __LINE__, file_name, strerror(errno)));
    }
    formatter_ = std::move(formatter);
}

bool FileSink::log(const LogRecord& record) noexcept 
{
    std::string inf;
    inf = formatter_->format(record);

    file_ << inf << '\n';
    if (file_.fail()) {
        std::cerr << __FILE__ << ":" << __LINE__ << " " << "Error writing to log file: " << std::strerror(errno) << '\n';
        file_.clear(); 
        return false;
    }

    if (record.level <= log_status::error) {
        return flush();
    }
    return true;
}

bool FileSink::flush() noexcept 
{
    file_.flush();
    if (file_.fail()) {
        std::cerr << __FILE__ << ":" << __LINE__ << " " << "Failed to flush file: " << std::strerror(errno) << '\n';
        file_.clear(); 
        return false;
    }
    return true;
}

Logger::Logger(const Logger& logger) noexcept 
{
    sinks_ = logger.sinks_;
}

Logger::Logger(Logger&& logger) noexcept 
{
    sinks_ = std::move(logger.sinks_);
}

Logger& Logger::operator=(const Logger& logger) noexcept 
{
    if (&logger == this) return *this;
    sinks_ = logger.sinks_;
    return *this;
}

Logger& Logger::operator=(Logger&& logger) noexcept 
{
    sinks_ = std::move(logger.sinks_);
    return *this;
}

bool Logger::addSink(std::shared_ptr<ILogSink> sink) noexcept 
{
    const char* sink_name = sink->name();
    for (auto&& existing : sinks_) {
        if (sink_name == existing->name()) {
            std::cerr << __FILE__ << ":" << __LINE__ << " " << "The same sink already existing" << '\n';
            return false;
        }
    }
    try {
        sinks_.push_back(std::move(sink));
    } catch(std::bad_alloc& mess) {
        std::cerr << __FILE__ << ":" << __LINE__ << " " << mess.what() << '\n';
        return false;
    }
    return true;
}

bool Logger::log(const LogRecord& record) noexcept 
{
    if (sinks_.empty()) {
        std::cerr << __FILE__ << ":" << __LINE__ << " " << "Sinks not added" << '\n';
        return false;
    }

    bool success = true;
    for (auto&& sink : sinks_) {
        if (!sink->log(record)) {
            success = false;
        }
    }
    flush();

    return success;
}

bool Logger::log(const log_status& level, std::string_view message) noexcept 
{
    return log(LogRecord{level, message});
}

bool Logger::log(const log_status& level, std::string_view message, std::string_view file) noexcept 
{
    return log(LogRecord{level, message, file});
}

bool Logger::log(const log_status& level, std::string_view message, 
         std::string_view file, int line) noexcept 
{
    return log(LogRecord{level, message, file, line});
}

bool Logger::log(const log_status& level, std::string_view message, 
         std::string_view file, int line, std::string_view func) noexcept 
{
    return log(LogRecord{level, message, file, line, func});
}

bool Logger::flush() noexcept 
{
    if (sinks_.empty()) {
        std::cerr << __FILE__ << ":" << __LINE__ << " " << "Sinks not added" << '\n';
        return false;
    }

    bool success = true;
    for (auto&& sink : sinks_) {
        if (!sink->flush()) {
            success = false;
        }
    }
    return success;
}

} // namespace logrr