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

    template <typename, typename=void>
    struct isStreamableImpl : std::false_type {};

    template <typename T> 
    struct isStreamableImpl<
        T,
        std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<T>())>
    > : std::true_type {};

    template <typename T>
    inline constexpr bool isStreamable = isStreamableImpl<T>::value;

    template <typename... Args>
    std::string concat(Args&&... args) {
        static_assert((isStreamable<Args> && ...), 
            "concat requires all arguments to support operator<<");
        std::ostringstream oss;
        (oss << ... << args);
        return oss.str();
    }
} // namespace


namespace http {

Server::Server() {    
    logger_.addSink(std::make_shared<logrr::ConsoleSink>());
    logger_.log(logrr::LogLevel::Info, "The \"Server\" constructor was called: Server object created");

    memset(&hints_, 0, sizeof(hints_));
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
        logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }
    return true;
}

bool Server::listenInternal(int max_connections, int bufsize) {
    int cli_sock;
    std::string buf;

    if (max_connections <= 0) {
        logger_.log(logrr::LogLevel::Error, "The number of connections must be greater than 0", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    if (bufsize <= 0) {
        logger_.log(logrr::LogLevel::Error, "The buffer size must be strictly greater than 0", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    int success;
    success = ::listen(sockfd_, max_connections);
    if (success == -1) {
        logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return false;
    }

    logger_.log(logrr::LogLevel::Info, "Server waiting for connections...");
    while(true) {
        cli_sock = acceptConnection(bufsize);
        if (cli_sock == -1) {
            logger_.log(logrr::LogLevel::Error, "Failed to create client socket");
            continue;
        }
        buf = receiveMessage(cli_sock, bufsize);
        logger_.log(logrr::LogLevel::Info, "Server received:");
        logger_.log(logrr::LogLevel::Info, concat('\n', buf));

        closeSocket(cli_sock);
        logger_.log(logrr::LogLevel::Info, "Client socket was closed");

        break;
    }
    return true;
}

int Server::acceptConnection(int bufsize) {
    int cli_sock;
    socklen_t cli_size;
    sockaddr_storage cli_addr;
    std::optional<std::string> cli_ip;
        
    cli_size = sizeof(cli_addr);
    cli_sock = accept(sockfd_, (sockaddr*)&cli_addr, &cli_size);
    if (cli_sock == -1) {
        logger_.log(logrr::LogLevel::Error, strerror(errno), __FILE__, __LINE__, __PRETTY_FUNCTION__);
        return -1;
    }
      
    cli_ip = getAddr((sockaddr*)&cli_addr);
    if (cli_ip) {
        logger_.log(logrr::LogLevel::Info, concat("Server got connection from: ", *cli_ip));
    }
    else {
        logger_.log(logrr::LogLevel::Warning, "The client's string address was not received", __FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return cli_sock;
}

std::string Server::receiveMessage(int sockfd, int bufsize) {
    int numbytes;
    char buf[bufsize];

    numbytes = recv(sockfd, buf, sizeof(buf), 0);
    logger_.log(logrr::LogLevel::Info, concat("Server received ", numbytes, " bytes"));
    return std::string(buf);
}

bool Server::addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept {
    return logger_.addSink(sink);
}

} // namespace http