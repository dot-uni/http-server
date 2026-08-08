#ifndef STATUS_LOGGING_INCLUDED
#define STATUS_LOGGING_INCLUDED

#include "logging.h"
#include "status.h"
#include "ret_status.h"

namespace logrr {

class StatusLogger : public Logger 
{
public:
    StatusLogger() = default;
    virtual ~StatusLogger() = default;
public:
    bool log(http::retCode code, const std::string& where) noexcept;
    bool log(http::retCode code, const std::string& where, std::vector<LogField>&& add_dtls) noexcept;
    bool log(http::retCode code, const std::string& where, int line, std::vector<LogField>&& add_dtls={}) noexcept;
    bool log(http::retCode code, const std::string& where, const std::string& file, std::vector<LogField>&& add_dtls={}) noexcept;
    bool log(http::retCode code, const std::string& where, const std::string& file, int line, std::vector<LogField>&& add_dtls={}) noexcept;

    bool log(http::status code, const std::string& where) noexcept;
    bool log(http::status code, const std::string& where, std::vector<LogField>&& add_dtls) noexcept;
    bool log(http::status code, const std::string& where, int line, std::vector<LogField>&& add_dtls={}) noexcept;
    bool log(http::status code, const std::string& where, const std::string& file, std::vector<LogField>&& add_dtls={}) noexcept;
    bool log(http::status code, const std::string& where, const std::string& file, int line, std::vector<LogField>&& add_dtls={}) noexcept;
};

} // namespace logrr 

#endif