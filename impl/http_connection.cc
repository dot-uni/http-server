#include "http_connection.h"

namespace http {

HttpConnection::HttpConnection(ClientConnection& client, int bufsize) : client_(std::move(client)), bufsize_(bufsize) {
    logger_.addSink(std::make_shared<logrr::ConsoleSink>());
}

HttpConnection::~HttpConnection() {
    closeConnection(client_.sockfd);
    logger_.log(logrr::LogRecord{
        .level = logrr::log_status::info,
        .message = "Client socket was closed",
        .fields {
            logrr::field("client_ip", client_.ip),
            logrr::field("client_port", client_.port)
        }
    });
}

void HttpConnection::closeConnection(int& sockfd) noexcept {
    if (sockfd > 0) {
        close(sockfd);
        sockfd = kEmptyDescriptor;
    }
    else sockfd = kInvalidSocket;
}

std::string HttpConnection::recvRawReq() noexcept {
    int numbytes, resbytes = 0;
    char buf[bufsize_];
    std::string request;

    while(true) {
        numbytes = recv(client_.sockfd, buf, sizeof(buf), 0);
        if (numbytes == -1) {
            logger_.log(logrr::LogRecord{
                .level = logrr::log_status::error,
                .message = "Unnable to recieve message",
                .file = __FILE__,
                .line = __LINE__,
                .fields {
                    logrr::field("errno", errno),
                    logrr::field("errno_str", strerror(errno))
                }
            }); 
            break;
        }
        else if (numbytes == 0) {
            logger_.log(logrr::log_status::warning, "Client disconnected");
            break; 
        }
        resbytes += numbytes;
        buf[numbytes] = '\0';
        request.append(buf);

        logger_.log(logrr::LogRecord{
            .level = logrr::log_status::info,
            .message = concat(numbytes, " bytes were received"),
            .fields {
                logrr::field("client_ip", client_.ip),
                logrr::field("client_port", client_.port)
            }
        });
        if (numbytes < bufsize_) break;
    } 
    logger_.log(logrr::LogRecord{
        .level = logrr::log_status::info,
        .message = concat("A total of ", resbytes, " bytes received from the client"),
        .fields {
            logrr::field("client_ip", client_.ip),
            logrr::field("client_port", client_.port)
        }
    });
    return request;
}

bool HttpConnection::recvReq() {
    std::string raw_req = recvRawReq();

    Request req;
    int end_targets = raw_req.find("\r\n");
    int end_headers = raw_req.find("\r\n\r\n");

    if (end_targets == std::string::npos || end_headers == std::string::npos) {
        logger_.log(logrr::LogRecord{
        .level = logrr::log_status::warning,
        .message = "Invalid request format",
        .fields {
            logrr::field("client_ip", client_.ip),
            logrr::field("client_port", client_.port),
            logrr::field("status_code", status::ok),
            logrr::field("status_code_mess", obsolete_reason(status::ok))
        }
        });
        return false;
    }

    std::string targets = raw_req.substr(0, end_targets);
    std::string headers = raw_req.substr(end_targets+2,  end_headers);
    req.body = raw_req.substr(end_headers+4);

    int first_space = targets.find(' ');
    int second_space = targets.find(' ', first_space+1);
    req.method = targets.substr(0, first_space);
    req.path = targets.substr(first_space, second_space);
    req.version = targets.substr(second_space);

    int beg = 0;
    int end = headers.find("\r\n"), colon;
    if (end == std::string::npos) return true;
    while(true) {
        colon = headers.find(":", beg);
        if (colon == std::string::npos || colon > end) {
            beg = end + 2;
            end = headers.find("\r\n", beg);
            if (end == std::string::npos) break;
            continue;
        }

        std::string key = headers.substr(beg, colon - beg);
        std::string value = headers.substr(colon + 1, end - colon - 1);

        size_t val_start = value.find_first_not_of(" \t");
        if (val_start != std::string::npos) value = value.substr(val_start);

        req.headers[key] = value;

        beg = end + 2;
        end = headers.find("\r\n", beg);
        if (end == std::string::npos) break;
    }

    req_ = std::move(req);
    return true;
}

} // namespace http