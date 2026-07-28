#include "http_server.h"


namespace {
    void* getSinAddr(sockaddr *sa) 
    {
        if (sa->sa_family == AF_INET) {
            return &(((sockaddr_in*)sa)->sin_addr);
        }
        else if (sa->sa_family == AF_INET6) {
            return &(((sockaddr_in6*)sa)->sin6_addr);
        }
        return nullptr;
    }

    uint16_t getSinPort(sockaddr *sa) 
    {
        if (sa->sa_family == AF_INET) {
            return (((sockaddr_in*)sa)->sin_port);
        }
        else if (sa->sa_family == AF_INET6) {
            return (((sockaddr_in6*)sa)->sin6_port);
        }
        return 0;
    }

    std::string getIpAddr(sockaddr* addr) 
    {
        char str_addr[INET6_ADDRSTRLEN];
        const char* success;

        success = inet_ntop(addr->sa_family, getSinAddr(addr), str_addr, sizeof(str_addr));
        if (success == nullptr) {
            return "";
        }
        return std::string(str_addr);
    }
} // namespace


namespace http {

HttpServer::HttpServer() 
{    
    logger_.addSink(std::make_shared<logrr::ConsoleSink>());
    logger_.log(logrr::log_status::info, "The HttpServer constructor was called: HttpServer object created");

    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = AF_UNSPEC;
    hints_.ai_socktype = SOCK_STREAM;
    hints_.ai_protocol = IPPROTO_TCP;
    hints_.ai_flags = AI_PASSIVE;
}

HttpServer::~HttpServer() 
{
    freeAddrInfo(servinfo_);
    closeConnection(sockfd_);
    logger_.log(logrr::log_status::info, "The HttpServer destructor was called: HttpServer has shut down");
}

void HttpServer::moveImpl(HttpServer& serv) noexcept 
{
    sockfd_ = std::move(serv.sockfd_);
    servinfo_ = std::move(serv.servinfo_);
    hints_ = std::move(serv.hints_);
    logger_ = std::move(serv.logger_);
    is_running_ = std::move(serv.is_running_);
}

HttpServer::HttpServer(HttpServer&& serv) noexcept 
{
    moveImpl(serv);
}

HttpServer& HttpServer::operator=(HttpServer&& serv) noexcept 
{
    if (&serv == this) return *this;
    freeAddrInfo(servinfo_);
    closeConnection(sockfd_);
    moveImpl(serv);
    return *this;
}

bool HttpServer::listen(const char* host, const char* port, int max_connections, int bufsize) 
{
    logger_.log(logrr::log_status::info, concat("The listen method was called, host=", host));
    return buildSocket(host, port) && listenInternal(max_connections, bufsize);
}

bool HttpServer::buildSocket(const char* host, const char* port) noexcept 
{
    int success, sockfd, opt = 1;
    addrinfo *servinfo, *p, *next;

    success = getaddrinfo(host, port, &hints_, &servinfo);
    if (success != 0) {
        logger_.log(logrr::LogRecord{
            .level = logrr::log_status::error,
            .message = "getaddrinfo() failed",
            .file = __FILE__,
            .line = __LINE__,
            .fields {
                logrr::field("gai_strerror", gai_strerror(success))
            }
        });
        return false;
    }

    for (p = servinfo; p != nullptr;) {
        next = p->ai_next;
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == kInvalidSocket) {
            logger_.log(logrr::LogRecord{
                .level = logrr::log_status::error,
                .message = "socket() failed",
                .file = __FILE__,
                .line = __LINE__,
                .fields {
                    logrr::field("errno", errno),
                    logrr::field("errno_str", strerror(errno))
                }
            });
            freeaddrinfo(p);
            p = next;
            continue;
        }

        success = setSockOptions(sockfd, SO_REUSEADDR, SO_REUSEPORT);
        if (success == -1) {
            logger_.log(logrr::LogRecord{
                .level = logrr::log_status::error,
                .message = "setSockOptions() failed",
                .file = __FILE__,
                .line = __LINE__,
                .fields {
                    logrr::field("errno", errno),
                    logrr::field("errno_str", strerror(errno))
                }
            });
            freeaddrinfo(p);
            closeConnection(sockfd);
            return false;
        }

        success = bind(sockfd, p->ai_addr, p->ai_addrlen);
        if (success == -1) {
            logger_.log(logrr::LogRecord{
                .level = logrr::log_status::error,
                .message = "bind() failed",
                .file = __FILE__,
                .line = __LINE__,
                .fields {
                    logrr::field("errno", errno),
                    logrr::field("errno_str", strerror(errno))
                }
            });
            freeaddrinfo(p);
            closeConnection(sockfd);
            p = next;
            continue;
        }
        break;
    }

    if (!p) {
        logger_.log(logrr::log_status::error, "HttpServer failed to bind", __FILE__, __LINE__);
        return false;
    }

    closeConnection(sockfd_);
    servinfo_ = std::move(p);
    sockfd_ = std::move(sockfd);
    logger_.log(logrr::log_status::info, "New socket successfully created");
    return true;
}

void HttpServer::freeAddrInfo(addrinfo*& servinfo) noexcept 
{
    freeaddrinfo(servinfo);
    servinfo = nullptr;
}

void HttpServer::closeConnection(int& sockfd) noexcept 
{
    if (sockfd > 0) {
        close(sockfd);
        sockfd = kEmptyDescriptor;
    }
    else sockfd = kInvalidSocket;
}

template <typename... Opts>
bool HttpServer::setSockOptions(int sockfd, Opts&&... args) noexcept 
{
    int opt = 1;
    bool success = (applyOption<Opts>(sockfd, std::forward<Opts>(args), opt), ...);
    return success;
}

template <typename Opt> bool HttpServer::applyOption(int sockfd, Opt&& arg, int opt) noexcept 
{
    int success;
    success = setsockopt(sockfd, SOL_SOCKET, arg, &opt, sizeof(opt));
    if (success == -1) {
        logger_.log(logrr::LogRecord{
            .level = logrr::log_status::error,
            .message = "setsockopt() failed",
            .file = __FILE__,
            .line = __LINE__,
            .fields {
                logrr::field("errno", errno),
                logrr::field("errno_str", strerror(errno))
            }
        });
        return false;
    }
    return true;
}

bool HttpServer::listenInternal(int max_connections, int bufsize) 
{
    int success;

    if (max_connections <= 0) {
        logger_.log(logrr::log_status::error, "The number of connections must be greater than 0", __FILE__, __LINE__);
        return false;
    }

    if (bufsize <= 0) {
        logger_.log(logrr::log_status::error, "The buffer size must be strictly greater than 0", __FILE__, __LINE__);
        return false;
    }

    success = ::listen(sockfd_, max_connections);
    if (success == -1) {
        logger_.log(logrr::LogRecord{
            .level = logrr::log_status::error,
            .message = "listen() failed",
            .file = __FILE__,
            .line = __LINE__,
            .fields {
                logrr::field("errno", errno),
                logrr::field("errno_str", strerror(errno))
            }
        });
        return false;
    }

    logger_.log(logrr::log_status::info, "HttpServer waiting for connections...");

    std::string raw_req;
    ClientConnection client;
    Request req;
    Response resp;
    while(true) {
        client = acceptConnection();
        if (client.sockfd == kInvalidSocket) continue;
        logger_.log(logrr::LogRecord{
            .level = logrr::log_status::info,
            .message = "Client connected",
            .fields {
                logrr::field("client_ip", client.ip),
                logrr::field("client_port", client.port)
            }
        });
        
        HttpConnection connection(client, bufsize);
        if (!connection.recvReq()) {
            continue;
        }
        break;
    }
    return true;
}

ClientConnection HttpServer::acceptConnection() 
{
    int cli_sock;
    socklen_t cli_size;
    sockaddr_storage cli_addr;
    std::string cli_ip;
    uint16_t cli_port;
    
    cli_size = sizeof(cli_addr);
    cli_sock = accept(sockfd_, (sockaddr*)&cli_addr, &cli_size);

    if (cli_sock == kInvalidSocket) {
        logger_.log(logrr::LogRecord{
            .level = logrr::log_status::error,
            .message = "accept() failed",
            .file = __FILE__,
            .line = __LINE__,
            .fields {
                logrr::field("errno", errno),
                logrr::field("errno_str", strerror(errno))
            }
        }); 
        return ClientConnection();
    }
      
    cli_ip = getIpAddr((sockaddr*)&cli_addr);
    cli_port = ntohs(getSinPort((sockaddr*)&cli_addr));

    return ClientConnection{generate_uuid_v4(), cli_sock, cli_ip, cli_port};
}

bool HttpServer::addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept 
{
    return logger_.addSink(sink);
}

} // namespace http