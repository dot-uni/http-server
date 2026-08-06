#ifndef LOG_STATUS_INCLUDED
#define LOG_STATUS_INCLUDED

#include <string_view>

namespace logrr {

/**
 * Codes used for Logging
 */

enum class log_status : int16_t 
{
    trace = 100,
    debug = 101,
    info = 102,
    warning = 103,
    error = 104,
    critical = 105
};

std::string_view obsolete_reason(log_status v);

} // namespace logrr 

#endif