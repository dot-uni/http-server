#include "http_server.h"

namespace {
    void* getInAddr(sockaddr *sa) {
        if (sa->sa_family == AF_INET) {
            return &(((sockaddr_in*)sa)->sin_addr);
        }
        else if (sa->sa_family == AF_INET6) {
            return &(((sockaddr_in6*)sa)->sin6_addr);
        }
        return nullptr;
    }

    std::optional<std::string> getAddr(sockaddr* addr) {
        char str_addr[INET6_ADDRSTRLEN];
        const char* success;

        success = inet_ntop(addr->sa_family, getInAddr(addr), str_addr, sizeof(str_addr));
        if (success == nullptr) {
            return std::nullopt;
        }
        return std::string(str_addr);
    }
} // namespace


namespace http {

Server::Server() {    
    logger_.addSink(std::make_shared<logrr::ConsoleSink>());
    logger_.log(logrr::LogLevel::Info, "The \"Server\" constructor was called: Server object created");
    hints_.ai_family = AF_UNSPEC;
    hints_.ai_socktype = SOCK_STREAM;
    hints_.ai_protocol = IPPROTO_TCP;
    hints_.ai_flags = AI_PASSIVE;
}

Server::~Server() {
    freeAddrInfo(servinfo_);
    closeSocket(sockfd_);
    logger_.log(logrr::LogLevel::Info, "The \"Server\" destructor was called: Server has shut down");
}

bool Server::listen(const char* host, const char* port, int max_connections, int bufsize) {
    try {
        std::string message = fmt::format("The listen method was called, host='{}'", host);
        logger_.log(logrr::LogLevel::Info, message);
    } catch(fmt::format_error& mess) {
        logger_.log(logrr::LogLevel::Warning, mess.what(), __FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return buildSocket(host, port) && listenInternal(max_connections, bufsize);
}

bool Server::buildSocket(const char* host, const char* port) noexcept {
    int success, sockfd, opt = 1;
    addrinfo *servinfo, *p, *next;

    success = getaddrinfo(host, port, &hints_, &servinfo);
    if (success != 0) {
        logger_.log(logrr::LogLevel::Error, gai_strerror(success), __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    for (p = servinfo; p != nullptr;) {
        next = p->ai_next;
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
            freeaddrinfo(p);
            p = next;
            continue;
        }

        success = setSockOptions(sockfd, SO_REUSEADDR, SO_REUSEPORT);
        if (success == -1) {
            logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
            freeaddrinfo(p);
            closeSocket(sockfd);
            return false;
        }

        success = bind(sockfd, p->ai_addr, p->ai_addrlen);
        if (success == -1) {
            logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
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

template <typename... Opt>
bool Server::setSockOptions(int sockfd, Opt&&... args) noexcept {
    int opt = 1;
    bool success = ((applyOption<Opt>(sockfd, std::forward<Opt>(args), opt)), ...);
    return success;
}

template <typename Opt> bool Server::applyOption(int sockfd, Opt&& arg, int opt) noexcept {
    int success;
    success = setsockopt(sockfd, SOL_SOCKET, arg, &opt, sizeof(opt));
    if (success == -1) {
        logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }
    return true;
}

bool Server::listenInternal(int max_connections, int bufsize) {
    if (max_connections <= 0) {
        logger_.log(logrr::LogLevel::Error, "The number of connections must be greater than 0", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    if (bufsize <= 0) {
        logger_.log(logrr::LogLevel::Error, "The buffer size must be strictly greater than 0", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    int success = ::listen(sockfd_, max_connections);
    if (success == -1) {
        logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }
    return acceptConnection(bufsize);
}

bool Server::acceptConnection(int bufsize) {
    int success, client, numbytes;
    socklen_t cli_size;
    sockaddr_storage cli_addr;
    std::optional<std::string> cli_ip;
    char buf[bufsize];

    logger_.log(logrr::LogLevel::Info, "Server waiting for connections...");
    while(true) {
        cli_size = sizeof(cli_addr);
        client = accept(sockfd_, (sockaddr*)&cli_addr, &cli_size);
        if (client == -1) {
            logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
            // continue;
        }
        
        cli_ip = getAddr((sockaddr*)&cli_addr);
        if (cli_ip) {
            try {
                std::string message = fmt::format("Server got connection from: {}", *cli_ip);
                logger_.log(logrr::LogLevel::Info, message);
            } catch(fmt::format_error& mess) {
                logger_.log(logrr::LogLevel::Warning, mess.what(), __FILE__, __LINE__, __PRETTY_FUNCTION__);
            }
        }
        else {
            logger_.log(logrr::LogLevel::Warning, "The client's string address was not received", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        }

        numbytes = recv(client, buf, sizeof(buf), 0);
        try {
            std::string message = fmt::format("Server received {} bytes", numbytes);
            logger_.log(logrr::LogLevel::Info, message);
        } catch(fmt::format_error& mess) {
            logger_.log(logrr::LogLevel::Warning, mess.what(), __FILE__, __LINE__, __PRETTY_FUNCTION__);
        }

        logger_.log(logrr::LogLevel::Info, "Server received:");
        logger_.log(logrr::LogLevel::Info, buf);

        closeSocket(client);
        logger_.log(logrr::LogLevel::Info, "Client socket was closed");
        break;
    }
    return true;
}

bool Server::addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept {
    return logger_.addSink(sink);
}

} // namespace http