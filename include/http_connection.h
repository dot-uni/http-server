#ifndef HTTP_CONNECTION_INCLUDED
#define HTTP_CONNECTION_INCLUDED

#include <sys/socket.h>
#include <unistd.h>
#include <random>
#include <sstream>
#include <iomanip>

#include "logging.h"
#include "http_codec.h"
#include "status.h"
#include "net_constants.h"


namespace http {

struct ClientConnection 
{
    std::string id;
    int sockfd = kInvalidSocket;
    std::string ip = "";
    uint16_t port = 0;
};


class HttpConnection {
public:
    HttpConnection(
        const ClientConnection& client, 
        int bufsize=kReceptionBufSize
    );
    HttpConnection(
        const ClientConnection& client, 
        std::shared_ptr<logrr::Logger> logger, 
        int bufsize=kReceptionBufSize
    );
    virtual ~HttpConnection();
    bool process();
protected:
    bool recv() noexcept;
    bool send(const Response& resp) noexcept;
    bool send() noexcept;
    bool execution();
    void closeConnection(int& sockfd) noexcept;
protected:
    ClientConnection client_; 
    std::string req_;
    std::string resp_;
    std::shared_ptr<logrr::Logger> logger_ = nullptr;
    int bufsize_;
};

} // namespace http

#endif