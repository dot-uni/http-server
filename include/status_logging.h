#ifndef STATUS_LOGGING_INCLUDED
#define STATUS_LOGGING_INCLUDED

#include "logging.h"
#include "status.h"

namespace logrr {

class StatusLogger : public Logger 
{
public:
    StatusLogger() = default;
    virtual ~StatusLogger() = default;
public:
    bool lOk(const std::string& where) noexcept;
    bool lOk(const std::string& where, std::vector<LogField>&& dtls) noexcept;

    bool lBadRequest(const std::string& where, std::vector<LogField>&& dtls) noexcept;
    bool lBadRequest(const std::string& where, int line, std::vector<LogField>&& dtls) noexcept;
    bool lBadRequest(const std::string& where, const std::string& file, std::vector<LogField>&& dtls) noexcept;
    bool lBadRequest(const std::string& where, const std::string& file, int line, std::vector<LogField>&& dtls) noexcept;
};


/// lOk
inline bool StatusLogger::lOk(const std::string& where) noexcept 
{
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>({
        logrr::field("status", http::status::ok),
        logrr::field("dstatus", http::obsolete_reason(http::status::ok))
    }));
}

inline bool StatusLogger::lOk(const std::string& where, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::ok),
        logrr::field("dstatus", http::obsolete_reason(http::status::ok))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls));
}


/// lBadRequest
inline bool StatusLogger::lBadRequest(const std::string& where, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::bad_request),
        logrr::field("dstatus", http::obsolete_reason(http::status::bad_request))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls));
}

inline bool StatusLogger::lBadRequest(const std::string& where, int line, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::bad_request),
        logrr::field("dstatus", http::obsolete_reason(http::status::bad_request))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", "", line);
}

inline bool StatusLogger::lBadRequest(const std::string& where, const std::string& file, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::bad_request),
        logrr::field("dstatus", http::obsolete_reason(http::status::bad_request))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", file);
}

inline bool StatusLogger::lBadRequest(const std::string& where, const std::string& file, int line, std::vector<LogField>&& add_dtls) noexcept
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::bad_request),
        logrr::field("dstatus", http::obsolete_reason(http::status::bad_request))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}

} // namespace logrr 

#endif