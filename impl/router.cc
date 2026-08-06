#include "router.h"

namespace http {

Router::Router(std::shared_ptr<logrr::StatusLogger> slogger) : slogger_(slogger) {}


void Router::add(const std::string& method, const std::string& path, Handler handler, bool auth_req=true) 
{

}

} // namespace http