#ifndef HTTP_CONNECTION
#define HTTP_CONNECTION

#include <sys/socket.h>
#include <unistd.h>

#include "logging.h"
#include "client_structures.h"


namespace http {

class HttpConnection {
public:
    HttpConnection(ClientConnection& client, int bufsize=DEF_RECEPTION_BUF_SIZE);
    virtual ~HttpConnection();
    bool recvReq();
    bool sendResp();
protected:
    std::string recvRawReq() noexcept;
    Response makeResp();
    void closeConnection(int& sockfd) noexcept;
private:
    ClientConnection client_; 
    Request req_;
    logrr::Logger logger_;
    int bufsize_;
};

} // namespace http

#endif