#ifndef HTTP_CODEC_INCLUDED
#define HTTP_CODEC_INCLUDED

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <memory>

#include "concat.h"
#include "status_logging.h"
#include "status.h"
#include "http_message.h"
#if 0
#include "router.h"
#endif

namespace http {


class HttpCodec
{
public:
    HttpCodec() = default;
    HttpCodec(std::shared_ptr<logrr::Logger>);
    virtual ~HttpCodec();
    std::string process(const std::string& raw_req);
protected:
    bool parse(const std::string&  raw_req) noexcept;
    bool serialize(Response& resp) noexcept;
protected:
    Request req_;
    std::string resp_;
    std::shared_ptr<logrr::StatusLogger> slogger_ = nullptr;
};


} // namespace http

#endif 