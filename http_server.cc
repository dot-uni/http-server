#include "http_server.h"


namespace {
    void* getSinAddr(sockaddr *sa) {
        if (sa->sa_family == AF_INET) {
            return &(((sockaddr_in*)sa)->sin_addr);
        }
        else if (sa->sa_family == AF_INET6) {
            return &(((sockaddr_in6*)sa)->sin6_addr);
        }
        return nullptr;
    }

    uint16_t getSinPort(sockaddr *sa) {
        if (sa->sa_family == AF_INET) {
            return (((sockaddr_in*)sa)->sin_port);
        }
        else if (sa->sa_family == AF_INET6) {
            return (((sockaddr_in6*)sa)->sin6_port);
        }
        return 0;
    }

    std::string getIpAddr(sockaddr* addr) {
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

Server::Server() {    
    logger_.addSink(std::make_shared<logrr::ConsoleSink>());
    logger_.log(logrr::LogLevel::Info, "The Server constructor was called: Server object created");

    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = AF_UNSPEC;
    hints_.ai_socktype = SOCK_STREAM;
    hints_.ai_protocol = IPPROTO_TCP;
    hints_.ai_flags = AI_PASSIVE;
}

Server::~Server() {
    freeAddrInfo(servinfo_);
    closeSocket(sockfd_);
    logger_.log(logrr::LogLevel::Info, "The Server destructor was called: Server has shut down");
}

void Server::moveImpl(Server& serv) noexcept {
    sockfd_ = std::move(serv.sockfd_);
    servinfo_ = std::move(serv.servinfo_);
    hints_ = std::move(serv.hints_);
    logger_ = std::move(serv.logger_);
    is_running_ = std::move(serv.is_running_);
}

Server::Server(Server&& serv) noexcept {
    moveImpl(serv);
}

Server& Server::operator=(Server&& serv) noexcept {
    if (&serv == this) return *this;
    freeAddrInfo(servinfo_);
    closeSocket(sockfd_);
    moveImpl(serv);
    return *this;
}

bool Server::listen(const char* host, const char* port, int max_connections, int bufsize) {
    logger_.log(logrr::LogLevel::Info, concat("The listen method was called, host=", host));
    return buildSocket(host, port) && listenInternal(max_connections, bufsize);
}

bool Server::buildSocket(const char* host, const char* port) noexcept {
    int success, sockfd, opt = 1;
    addrinfo *servinfo, *p, *next;

    success = getaddrinfo(host, port, &hints_, &servinfo);
    if (success != 0) {
        logger_.log(logrr::LogRecord{
            .level = logrr::LogLevel::Error,
            .message = "getaddrinfo() failed",
            .file = __FILE__,
            .line = __LINE__,
            .fields {
                {"gai_strerror", gai_strerror(success)},
            }
        });
        return false;
    }

    for (p = servinfo; p != nullptr;) {
        next = p->ai_next;
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            logger_.log(logrr::LogRecord{
                .level = logrr::LogLevel::Error,
                .message = "socket() failed",
                .file = __FILE__,
                .line = __LINE__,
                .fields {
                    {"errno", errno},
                    {"errno_str", strerror(errno)}
                }
            });
            freeaddrinfo(p);
            p = next;
            continue;
        }

        success = setSockOptions(sockfd, SO_REUSEADDR, SO_REUSEPORT);
        if (success == -1) {
            logger_.log(logrr::LogRecord{
                .level = logrr::LogLevel::Error,
                .message = "setSockOptions() failed",
                .file = __FILE__,
                .line = __LINE__,
                .fields {
                    {"errno", errno},
                    {"errno_str", strerror(errno)}
                }
            });
            freeaddrinfo(p);
            closeSocket(sockfd);
            return false;
        }

        success = bind(sockfd, p->ai_addr, p->ai_addrlen);
        if (success == -1) {
            logger_.log(logrr::LogRecord{
                .level = logrr::LogLevel::Error,
                .message = "bind() failed",
                .file = __FILE__,
                .line = __LINE__,
                .fields {
                    {"errno", errno},
                    {"errno_str", strerror(errno)}
                }
            });
            freeaddrinfo(p);
            closeSocket(sockfd);
            p = next;
            continue;
        }
        break;
    }

    if (!p) {
        logger_.log(logrr::LogLevel::Error, "Server failed to bind", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    closeSocket(sockfd_);
    servinfo_ = std::move(p);
    sockfd_ = std::move(sockfd);
    logger_.log(logrr::LogLevel::Info, "New socket successfully created");
    return true;
}

void Server::freeAddrInfo(addrinfo*& servinfo) noexcept {
    freeaddrinfo(servinfo);
    servinfo = nullptr;
}

void Server::closeSocket(int& sockfd) noexcept {
    if (sockfd > 0) {
        close(sockfd);
        sockfd = kEmptyDescriptor;
    }
    else sockfd = kInvalidSocket;
}

template <typename... Opts>
bool Server::setSockOptions(int sockfd, Opts&&... args) noexcept {
    int opt = 1;
    bool success = (applyOption<Opts>(sockfd, std::forward<Opts>(args), opt), ...);
    return success;
}

template <typename Opt> bool Server::applyOption(int sockfd, Opt&& arg, int opt) noexcept {
    int success;
    success = setsockopt(sockfd, SOL_SOCKET, arg, &opt, sizeof(opt));
    if (success == -1) {
        logger_.log(logrr::LogRecord{
            .level = logrr::LogLevel::Error,
            .message = "setsockopt() failed",
            .file = __FILE__,
            .line = __LINE__,
            .fields {
                {"errno", errno},
                {"errno_str", strerror(errno)}
            }
        });
        return false;
    }
    return true;
}

bool Server::listenInternal(int max_connections, int bufsize) {
    int success;

    if (max_connections <= 0) {
        logger_.log(logrr::LogLevel::Error, "The number of connections must be greater than 0", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    if (bufsize <= 0) {
        logger_.log(logrr::LogLevel::Error, "The buffer size must be strictly greater than 0", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    success = ::listen(sockfd_, max_connections);
    if (success == -1) {
        logger_.log(logrr::LogRecord{
            .level = logrr::LogLevel::Error,
            .message = "listen() failed",
            .file = __FILE__,
            .line = __LINE__,
            .fields {
                {"errno", errno},
                {"errno_str", strerror(errno)}
            }
        });
        return false;
    }

    logger_.log(logrr::LogLevel::Info, "Server waiting for connections...");

    std::string req;
    ClientConnection client;
    while(true) {
        client = acceptConnection();
        if (client.sockfd == -1) continue;
        logger_.log(logrr::LogRecord{
            .level = logrr::LogLevel::Info,
            .message = "Client connected",
            .fields {
                {"client_ip", client.ip},
                {"client_port", client.port}
            }
        });

        req = receiveMessage(client, bufsize);
        parseReq(client, req);

        closeSocket(client.sockfd);
        logger_.log(logrr::LogRecord{
            .level = logrr::LogLevel::Info,
            .message = "Client socket was closed",
            .fields {
                {"client_ip", client.ip},
                {"client_port", client.port}
            }
        });
        break;
    }
    return true;
}

ClientConnection Server::acceptConnection() {
    int cli_sock;
    socklen_t cli_size;
    sockaddr_storage cli_addr;
    std::string cli_ip;
    uint16_t cli_port;
    
    cli_size = sizeof(cli_addr);
    cli_sock = accept(sockfd_, (sockaddr*)&cli_addr, &cli_size);

    if (cli_sock == -1) {
        logger_.log(logrr::LogRecord{
            .level = logrr::LogLevel::Error,
            .message = "accept() failed",
            .file = __FILE__,
            .line = __LINE__,
            .fields {
                {"errno", errno},
                {"errno_str", strerror(errno)}
            }
        }); 
        return ClientConnection{-1, "", 0};
    }
      
    cli_ip = getIpAddr((sockaddr*)&cli_addr);
    cli_port = ntohs(getSinPort((sockaddr*)&cli_addr));

    return ClientConnection{cli_sock, cli_ip, cli_port};
}

std::string Server::receiveMessage(ClientConnection& client, int bufsize) {
    int numbytes, resbytes = 0;
    char buf[bufsize];
    std::string request;

    while(true) {
        numbytes = recv(client.sockfd, buf, sizeof(buf), 0);
        if (numbytes == -1) {
            logger_.log(logrr::LogRecord{
                .level = logrr::LogLevel::Error,
                .message = "Unnable to recieve message",
                .file = __FILE__,
                .line = __LINE__,
                .fields {
                    {"errno", errno},
                    {"errno_str", strerror(errno)}
                }
            }); 
            break;
        }
        else if (numbytes == 0) {
            logger_.log(logrr::LogLevel::Warning, "Client disconnected");
            break; 
        }
        resbytes += numbytes;
        buf[numbytes] = '\0';
        request.append(buf);

        logger_.log(logrr::LogRecord{
            .level = logrr::LogLevel::Info,
            .message = concat(numbytes, " bytes were received"),
            .fields {
                {"client_ip", client.ip},
                {"client_port", client.port}
            }
        });
        if (numbytes < bufsize) break;
    } 
    logger_.log(logrr::LogRecord{
        .level = logrr::LogLevel::Info,
        .message = concat("A total of ", resbytes, " bytes received from the client"),
        .fields {
            {"client_ip", client.ip},
            {"client_port", client.port}
        }
    });
    return request;
}

bool Server::addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept {
    return logger_.addSink(sink);
}

bool Server::parseReq(ClientConnection& client, std::string& req) {
    int end_targets = req.find("\r\n");
    int end_headers = req.find("\r\n\r\n");

    if (end_targets == std::string::npos || end_headers == std::string::npos) {
        logger_.log(logrr::LogRecord{
            .level = logrr::LogLevel::Warning,
            .message = "Invalid request format",
            .fields {
                {"client_ip", client.ip},
                {"client_port", client.port},
                {"status_code", 400}
            }
        });
        return false;
    }
    std::string targets = req.substr(0, end_targets);
    std::string headers = req.substr(end_targets,  end_headers);
    std::string body = req.substr(end_headers);
    return true;
}

} // namespace http