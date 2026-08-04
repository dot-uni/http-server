#include "log_status.h"

namespace logrr {

std::string_view obsolete_reason(log_status v) 
{
    switch(static_cast<log_status>(v)) {
        case log_status::trace:                         return "TRACE";
        case log_status::debug:                         return "DEBUG";
        case log_status::info:                          return "INFO";
        case log_status::warning:                       return "WARN";
        case log_status::error:                         return "ERROR";
        case log_status::critical:                      return "CRIT";
        default:
            break;
    }
    return "<unknown-log_status>";
}

} // namespace logrr