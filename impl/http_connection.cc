#include "http_connection.h"

namespace http {


HttpConnection::HttpConnection(
    const ClientConnection& client, 
    int bufsize
) : client_(std::move(client)), bufsize_(bufsize) {
    if (logger_) logger_->lExeced(__func__);
}


HttpConnection::HttpConnection(
    const ClientConnection& client, 
    std::shared_ptr<logrr::Logger> logger, 
    int bufsize
) : client_(std::move(client)), logger_(logger), bufsize_(bufsize) {
    if (logger_) logger_->lExeced(__func__);
}


HttpConnection::~HttpConnection() 
{
    if (logger_) logger_->lCalled(__func__);

    closeConnection(client_.sockfd);

    if (logger_) logger_->lExeced(__func__, {
        logrr::field("message", "Client socket was closed"),
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port)
    });
}

bool HttpConnection::process() 
{
    if (logger_) logger_->lCalled(__func__);
    return HttpConnection::recv() && execution() /*&& HttpConnection::send()*/;
}


bool HttpConnection::recv() noexcept 
{
    if (logger_) logger_->lCalled(__func__);

    int numbytes, resbytes = 0;
    char buf[bufsize_];
    std::string req;

    while(true) {
        numbytes = ::recv(client_.sockfd, buf, sizeof(buf), 0);
        if (numbytes == -1) {
            if (logger_) logger_->lError(__func__, __LINE__, {
                logrr::field("message", "Error from ::recv"),
                logrr::field("errno", errno),
                logrr::field("errno_str", strerror(errno))
            });
            return false;
        }
        else if (numbytes == 0) {
            if (logger_) logger_->lWarning(__func__, __LINE__, {
                logrr::field("message", "Client disconnected"),
            }); 
            return false;
        }

        resbytes += numbytes;
        buf[numbytes] = '\0';
        req.append(buf);

        if (logger_) logger_->lInfo(__func__, {
            logrr::field("message", concat(numbytes, " bytes were received")),
            logrr::field("client_id", client_.id),
            logrr::field("client_ip", client_.ip),
            logrr::field("client_port", client_.port)
        });

        if (numbytes < bufsize_) break;
    } 

    req_ = std::move(req);
    
    if (logger_) logger_->lExeced(__func__, {
        logrr::field("message", concat("A total of ", resbytes, " bytes received from the client")),
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port)
    });
    return true;
}


bool HttpConnection::execution() {
    HttpCodec codec;
    resp_ = codec.process(req_); 
    return true;
}

#if 0
bool HttpConnection::send() noexcept 
{
    int numbytes;
    while(true) {
        numbytes = ::send(client_, )
    }
}
#endif 

void HttpConnection::closeConnection(int& sockfd) noexcept 
{
    if (logger_) logger_->lCalled(__func__);

    if (sockfd > 0) {
        close(sockfd);
        sockfd = kEmptyDescriptor;
    }
    else sockfd = kInvalidSocket;

    if (logger_) logger_->lExeced(__func__);
}

} // namespace http