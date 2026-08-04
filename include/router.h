#ifndef ROUTER_INCLUDED
#define ROUTER_INCLUDED

#include <string>
#include <vector>
#include <functional>

#include "http_message.h"

namespace http {

using Handler = std::function<Response(const Request&)>;

struct Route {
    std::string method;
    std::string path;
    Handler handler;
};

class Router {

};

} // namespace http

#endif