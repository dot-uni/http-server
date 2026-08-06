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

} // namespace logrr 

#endif