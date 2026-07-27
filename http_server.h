#ifndef HTTP_SERVER
#define HTTP_SERVER

#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string_view>
#include <memory>
#include <mutex>
#include <sstream>
#include <type_traits>

#include "concat.h"
#include "logging.h"
#include "http_connection.h"
#include "client_structures.h"

namespace http {


class HttpServer {
public:
    HttpServer();
    virtual ~HttpServer();
    HttpServer(HttpServer&& serv) noexcept;
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(HttpServer&& serv) noexcept;
    HttpServer& operator=(const HttpServer&) = delete;

    bool listen();
    bool listen(const char* host, const char* port=DEF_HTTP_PORT, int max_connections=DEF_MAX_CONNECTIONS, int bufsize=DEF_RECEPTION_BUF_SIZE);
    bool continueListen();
    void stopListen() noexcept;
    bool addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept;
protected:
    bool buildSocket(const char* host, const char* port) noexcept;
    void freeAddrInfo(addrinfo*& servinfo) noexcept;
    void closeConnection(int& sockfd) noexcept;
    template <typename... Opts> bool setSockOptions(int sockfd, Opts&&... args) noexcept;
    template <typename Opt> bool applyOption(int sockfd, Opt&& arg, int opt) noexcept;
    bool listenInternal(int max_connections, int bufsize);
    ClientConnection acceptConnection();
    void moveImpl(HttpServer& serv) noexcept;
private:
    int sockfd_ = kEmptyDescriptor;
    addrinfo* servinfo_ = nullptr;
    addrinfo hints_;
    logrr::Logger logger_;
    bool is_running_ = false;
    std::mutex mtx_;
};

inline bool HttpServer::listen() { return listen("0.0.0.0"); }
inline void HttpServer::stopListen() noexcept { closeConnection(sockfd_); }

} // namespace http

#endif