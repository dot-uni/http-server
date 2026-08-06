#ifndef ROUTER_INCLUDED
#define ROUTER_INCLUDED

#include <string>
#include <vector>
#include <unoredered_map>
#include <functional>

#include "status_logging.h"
#include "http_message.h"

namespace http {

using Handler = std::function<Response(const Request&)>;

class Router {
public:
    Router() = default;
    Router(std::shared_ptr<logrr::StatusLogger>);
    virtual ~Router() = default;

    Response route(const Request& req) const noexcept;
    void add(const std::string& method, const std::string& path, Handler handler, bool auth_req=true);
protected:
    std::unoredered_map<std::string, Handler> available_funcs_;
    std::shared_ptr<logrr::StatusLogger> slogger_ = nullptr;
};

} // namespace http

#endif