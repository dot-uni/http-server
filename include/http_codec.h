#ifndef HTTP_CODEC_INCLUDED
#define HTTP_CODEC_INCLUDED

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <memory>

#include "status_logging.h"
#include "status.h"
#include "http_message.h"
#include "tostring.h"
#if 0
#include "router.h"
#endif

namespace http {


class HttpCodec
{
public:
    HttpCodec() = default;
    HttpCodec(std::shared_ptr<logrr::Logger>);
    virtual ~HttpCodec() = default;
    std::string process(const std::string& raw_req, const std::string& id);
protected:
    bool parse(const std::string& raw_req) noexcept;
    std::string serialize(Response& resp) noexcept;
protected:
    Request req_;
    std::shared_ptr<logrr::StatusLogger> slogger_ = nullptr;
};


} // namespace http

#endif 