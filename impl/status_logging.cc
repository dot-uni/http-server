#include "status_logging.h"

namespace logrr {


/**
 * log with RetCode
 */

bool StatusLogger::log(http::retCode code, const std::string& where) noexcept
{
    http::status s = http::toHttpStatus(code);
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>({
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    }));
}


bool StatusLogger::log(http::retCode code, const std::string& where, std::vector<LogField>&& add_dtls) noexcept
{
    http::status s = http::toHttpStatus(code);
    std::vector<LogField> dtls = {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls));
}


bool StatusLogger::log(http::retCode code, const std::string& where, int line, std::vector<LogField>&& add_dtls) noexcept 
{
    http::status s = http::toHttpStatus(code);
    std::vector<LogField> dtls = {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), "", "", line);
}


bool StatusLogger::log(http::retCode code, const std::string& where, const std::string& file, std::vector<LogField>&& add_dtls) noexcept 
{
    http::status s = http::toHttpStatus(code);
    std::vector<LogField> dtls = {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), "", file);
}


bool StatusLogger::log(http::retCode code, const std::string& where, const std::string& file, int line, std::vector<LogField>&& add_dtls) noexcept
{
    http::status s = http::toHttpStatus(code);
    std::vector<LogField> dtls = {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}


/**
 * log with HTTP status
 */

bool StatusLogger::log(http::status code, const std::string& where) noexcept
{
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>({
        logrr::field("status", code),
        logrr::field("obsolete_reason", http::obsolete_reason(code))
    }));
}


bool StatusLogger::log(http::status code, const std::string& where, std::vector<LogField>&& add_dtls) noexcept
{
    std::vector<LogField> dtls = {
        logrr::field("status", code),
        logrr::field("obsolete_reason", http::obsolete_reason(code))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return standardTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls));
}


bool StatusLogger::log(http::status code, const std::string& where, int line, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", code),
        logrr::field("obsolete_reason", http::obsolete_reason(code))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), "", "", line);
}


bool StatusLogger::log(http::status code, const std::string& where, const std::string& file, std::vector<LogField>&& add_dtls) noexcept 
{
    std::vector<LogField> dtls = {
        logrr::field("status", code),
        logrr::field("obsolete_reason", http::obsolete_reason(code))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), "", file);
}


bool StatusLogger::log(http::status code, const std::string& where, const std::string& file, int line, std::vector<LogField>&& add_dtls) noexcept
{
    std::vector<LogField> dtls = {
        logrr::field("status", code),
        logrr::field("obsolete_reason", http::obsolete_reason(code))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return detailedTempl(logrr::log_status::info, where, std::forward<std::vector<LogField>>(dtls), "", file, line);
}

} // namespace logrr