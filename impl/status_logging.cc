#include "status_logging.h"

namespace logrr {

/// lOk
bool StatusLogger::lOk(const std::string& where) noexcept 
{
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>({
        logrr::field("status", http::status::ok),
        logrr::field("dstatus", http::obsolete_reason(http::status::ok))
    }));
}

bool StatusLogger::lOk(const std::string& where, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::ok),
        logrr::field("dstatus", http::obsolete_reason(http::status::ok))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls));
}


/// lBadRequest
bool StatusLogger::lBadRequest(const std::string& where, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::bad_request),
        logrr::field("dstatus", http::obsolete_reason(http::status::bad_request))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls));
}

bool StatusLogger::lBadRequest(const std::string& where, int line, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::bad_request),
        logrr::field("dstatus", http::obsolete_reason(http::status::bad_request))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", "", line);
}

bool StatusLogger::lBadRequest(const std::string& where, const std::string& file, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::bad_request),
        logrr::field("dstatus", http::obsolete_reason(http::status::bad_request))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", file);
}

bool StatusLogger::lBadRequest(const std::string& where, const std::string& file, int line, std::vector<LogField>&& add_dtls) noexcept
{
    std::vector<LogField> dtls = {
        logrr::field("status", http::status::bad_request),
        logrr::field("dstatus", http::obsolete_reason(http::status::bad_request))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::error, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}

} // namespace logrr