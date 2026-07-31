#include "logging.h"


namespace logrr {


/** logrr::SingleLineFormatter Implementation
 */

std::string SingleLineFormatter::format(const LogRecord& r) const noexcept 
{
    std::string base = "";
    try {
        base = fmt::format("{} [{}] – {}", 
            r.timepoint, logrr::obsolete_reason(r.status), r.message);
        if (!r.file.empty()) {
            base += fmt::format(R"(, "file": "{}")", r.file);
        }
        if (r.line) {
            base += fmt::format(R"(, "line": {})", r.line);
        }
        for (auto&& detail : r.details) {
            base += fmt::format(R"(, "{}": {})", detail.first, detail.second);
        }
    }
    catch(fmt::format_error& mess) 
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " << mess.what() << '\n';
    }
    return base;
}


/** logrr::JsonFormatter Implementation
 */

std::string JsonFormatter::format(const LogRecord& r) const noexcept 
{
    return json::obj(
        json::field("timepoint", r.timepoint),
        json::field("status_code", static_cast<int>(r.status)),
        json::field("status", logrr::obsolete_reason(r.status)),
        json::field("message", r.message),
        json::field("file", r.file),
        json::field("line", r.line),
        json::field("details", 
            json::arr(
                r.details
            )        
        )
    );
}


/** logrr::ConsoleSink Implementation
 */

ConsoleSink::ConsoleSink() : 
formatter_(std::make_shared<SingleLineFormatter>()) {}

ConsoleSink::ConsoleSink(std::shared_ptr<ILogFormatter> formatter) : 
formatter_(std::move(formatter)) {}

bool ConsoleSink::log(const LogRecord& record) noexcept 
{
    std::string inf;
    inf = formatter_->format(record);

    std::ostream& out = (record.status <= log_status::error) ? std::cerr : std::cout;
    out << inf << '\n';

    return static_cast<bool>(out);
}


/** logrr::FileSink Implementation
 */

FileSink::FileSink(const std::string& file_name, std::shared_ptr<ILogFormatter> formatter) 
{
    file_.open(file_name, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error(fmt::format("{}:{} Failed to open file '{}': {}", 
                                    __FILE__, __LINE__, file_name, strerror(errno)));
    }
    formatter_ = std::move(formatter);
}

FileSink::FileSink(const std::string& file_name) :
FileSink(file_name, std::make_shared<JsonFormatter>()) {}

FileSink::FileSink() : 
FileSink(fmt::format("log_{}.log", timeToString(std::chrono::system_clock::now())), std::make_shared<JsonFormatter>()) {}

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

    if (record.status <= log_status::error) {
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


/** logrr::Logger Implementation 
 */

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


inline bool Logger::simple_templ(
    log_status status,
    const std::string& where, 
    std::vector<LogField>&& dtls, 
    const std::string& sentence_prefix
) noexcept 
{
    return log(LogRecord{
        .status = status,
        .message = where + sentence_prefix,
        .details = std::forward<std::vector<LogField>>(dtls)
    });
}


inline bool Logger::detailed_templ(
    log_status status,
    const std::string& where, 
    std::vector<LogField>&& dtls, 
    const std::string& sentence_prefix,
    const std::string& file, 
    int line
) noexcept 
{
    return log(LogRecord{
        .status = status,
        .message = where + sentence_prefix,
        .file = file,
        .line = line,
        .details = std::forward<std::vector<LogField>>(dtls)
    });
}


inline bool Logger::linfo(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return simple_templ(logrr::log_status::info, where , std::forward<std::vector<LogField>>(dtls));
}

inline bool Logger::lcalled(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return simple_templ(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), " was called");
}

inline bool Logger::lexeced(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return simple_templ(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), " executed successfully");
}


inline bool Logger::lfailed(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), " failed");
}

inline bool Logger::lfailed(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), " failed", "", line);
}

inline bool Logger::lfailed(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), " failed", file);
}

inline bool Logger::lfailed(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailed_templ(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), " failed", file, line);
}


inline bool Logger::lerror(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls));
}

inline bool Logger::lerror(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", "", line);
}

inline bool Logger::lerror(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", file);
}

inline bool Logger::lerror(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailed_templ(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


inline bool Logger::lwarning(const std::string& where, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::warning, where, std::forward<std::vector<LogField>>(dtls));
}

inline bool Logger::lwarning(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::warning, where, std::forward<std::vector<LogField>>(dtls), "", "", line);
}

inline bool Logger::lwarning(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept 
{
    return detailed_templ(logrr::log_status::warning, where, std::forward<std::vector<LogField>>(dtls), "", file);
}

inline bool Logger::lwarning(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailed_templ(logrr::log_status::warning, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


inline bool Logger::lcritical(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailed_templ(logrr::log_status::critical, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


inline bool Logger::ldebug(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailed_templ(logrr::log_status::debug, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


inline bool Logger::ltrace(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept
{
    return detailed_templ(logrr::log_status::trace, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
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