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
#include <map>
#include <sys/wait.h>
#include <signal.h>
#include <string_view>
#include <memory>
#include <mutex>

#include "logging.h"

#define DEF_HTTP_PORT "8080"
#define DEF_MAX_CONNECTIONS 20
#define DEF_RECEPTION_BUF_SIZE 32768


namespace http {

constexpr int kInvalidSocket = -1;
constexpr int kEmptyDescriptor = 0;

#if 0
struct Request {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> header_map;
    std::string body;
};

struct Response {
    
};
#endif

class Server {
public:
    Server();
    virtual ~Server();
    bool listen();
    bool listen(const char* host, const char* port=DEF_HTTP_PORT, int max_connections=DEF_MAX_CONNECTIONS, int bufsize=DEF_RECEPTION_BUF_SIZE);
    bool continueListen();
    void stopListen() noexcept;
    bool addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept;
protected:
    bool buildSocket(const char* host, const char* port) noexcept;
    void freeAddrInfo(addrinfo*& servinfo) noexcept;
    void closeSocket(int& sockfd) noexcept;
    template <typename... Opt> bool setSockOptions(int sockfd, Opt&&... args) noexcept;
    template <typename Opt> bool applyOption(int sockfd, Opt&& arg, int opt) noexcept;
    bool listenInternal(int max_connections, int bufsize);
    bool acceptConnection(int bufsize);

private:
    int sockfd_ = kEmptyDescriptor;
    addrinfo* servinfo_ = nullptr;
    addrinfo hints_;
    logrr::Logger logger_;
    bool is_decommissioned_ = false;
    bool is_running_ = false;
    std::mutex mtx_;
};

inline bool Server::listen() { return listen("0.0.0.0"); }
inline void Server::stopListen() noexcept { closeSocket(sockfd_); }

} // namespace http

#endif