#ifndef LOG_STATUS
#define LOG_STATUS

#include <string_view>

namespace logrr {

enum class log_status : int16_t 
{
    trace = 102,
    debug = 101,
    info = 100,
    warning = 0,
    error = -100,
    critical = -101
};

std::string_view obsolete_reason(log_status v);

} // namespace logrr 

#endif