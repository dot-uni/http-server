#include "logging.h"


namespace logrr {

std::string SingleLineFormatter::format(const LogRecord& r) const noexcept {
    if (r.level.num() <= LogLevel::Warning.num()) {
        return formatDetailed(r);
    }
    return formatCompact(r);
}

std::string SingleLineFormatter::formatCompact(const LogRecord& r) const noexcept {
    std::string base = "";
    try {
        base = fmt::format("{} [{}] – {}", 
            r.timepoint, r.level.name(), r.message);
    }
    catch(fmt::format_error& mess) {
        std::cerr << mess.what() << '\n';
    }
    return base;
}

std::string SingleLineFormatter::formatDetailed(const LogRecord& r) const noexcept {
    std::string base = formatCompact(r);
    try {
        if (r.file != "none") 
            base += fmt::format(" - \"{}\"", r.file);
        if (r.line) 
            base += fmt::format(":{}", r.line);
        if (r.func != "none") 
            base += fmt::format(" [{}]", r.func);
    } catch(fmt::format_error& mess) {
        std::cerr << mess.what() << '\n';
    }
    return base; 
}

std::string JsonFormatter::format(const LogRecord& r) const noexcept {
    if (r.level.num() <= LogLevel::Warning.num()) {
        return formatDetailed(r);
    }
    return formatCompact(r);
}

std::string JsonFormatter::formatCompact(const LogRecord& r) const noexcept {
    std::string base = "";
    try {
        base = fmt::format(R"({{"timepoint":"{}","level":"{}","message":"{}"}})", 
            r.timepoint, r.level.name(), r.message);
    } catch(fmt::format_error& mess) {
        std::cerr << mess.what() << '\n';
    }
    return base;
}

std::string JsonFormatter::formatDetailed(const LogRecord& r) const noexcept {
    std::string base = formatCompact(r);
    try {
        base += fmt::format(R"("file":"{}","line":"{}","function":"{}")", 
            r.file, r.line, r.func);
    } catch(fmt::format_error& mess) {
        std::cerr << mess.what() << '\n';
    }
    return base;
}

bool ConsoleSink::log(const LogRecord& record) noexcept {
    std::string inf;
    try {
        inf = formatter_->format(record);
    } catch(...) {
        return false;
    }

    // std::lock_guard<std::mutex> lock(mtx_);
    std::ostream& out = (record.level.num() <= LogLevel::Error.num()) ? std::cerr : std::cout;
    out << inf << '\n';

    return static_cast<bool>(out);
}

FileSink::FileSink(const std::string& file_name, std::shared_ptr<ILogFormatter> formatter) {
    file_.open(file_name, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file '{}': {}", 
                                    file_name, strerror(errno)));
    }
    formatter_ = std::move(formatter);
}

bool FileSink::log(const LogRecord& record) noexcept {
    std::string inf;
    try {
        inf = formatter_->format(record);
    } catch(std::exception&) {
        return false;
    }

    // std::lock_guard<std::mutex> lock(mtx_);
    file_ << inf << '\n';
    if (file_.fail()) {
        std::cerr << "Error writing to log file: " << std::strerror(errno) << '\n';
        file_.clear(); 
        return false;
    }

    if (record.level.num() <= LogLevel::Error.num()) {
        return flush();
    }
    return true;
}

bool FileSink::flush() noexcept {
    file_.flush();
    if (file_.fail()) {
        std::cerr << "Failed to flush file: " << std::strerror(errno) << '\n';
        file_.clear(); 
        return false;
    }
    return true;
}

bool Logger::addSink(std::shared_ptr<ILogSink> sink) noexcept {
    // std::lock_guard<std::mutex> lock(mtx_);
    const char* sink_name = sink->name();
    for (auto&& existing : sinks_) {
        if (sink_name == existing->name()) {
            std::cerr << "The same sink already existing" << '\n';
            return false;
        }
    }
    try {
        sinks_.push_back(std::move(sink));
    } catch(std::bad_alloc& mess) {
        std::cerr << mess.what() << '\n';
        return false;
    }
    return true;
}

bool Logger::log(const LogRecord& record) noexcept {
    // std::lock_guard<std::mutex> lock(mtx_);

    if (sinks_.empty()) {
        std::cerr << "Sinks not added" << '\n';
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

bool Logger::log(const LogLevel& level, const std::string& message) noexcept {
    return log(LogRecord{level, message});
}

bool Logger::log(const LogLevel& level, const std::string& message, const std::string& file) noexcept {
    return log(LogRecord{level, message, file});
}

bool Logger::log(const LogLevel& level, const std::string& message, 
         const std::string& file, int line) noexcept {
    return log(LogRecord{level, message, file, line});
}

bool Logger::log(const LogLevel& level, const std::string& message, 
         const std::string& file, int line, const std::string& func) noexcept {
    return log(LogRecord{level, message, file, line, func});
}

bool Logger::flush() noexcept {
    // std::lock_guard<std::mutex> lock(mtx_);
    if (sinks_.empty()) {
        std::cerr << "Sinks not added" << '\n';
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