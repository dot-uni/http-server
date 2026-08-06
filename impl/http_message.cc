#include "http_message.h"

namespace http {

int64_t get_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

Response makeResp(http::retCode retcode, const std::string& id, const nlohmann::ordered_json& result) 
{
    return Response{
        .status = toHttpStatus(retcode),
        .headers {
            {"X-Request-Id", id}
        },
        .body = {
            {"retCode", retcode},
            {"retMesg", retMesg(retcode)},
            {"time", get_timestamp_ms()},
            {"result", result}
        }
    };
}

} // namespace http