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
#include <sstream>
#include <type_traits>

#include "concat.h"
#include "logging.h"

#define DEF_HTTP_PORT "8080"
#define DEF_MAX_CONNECTIONS 20
#define DEF_RECEPTION_BUF_SIZE 32768

namespace http {

constexpr int kInvalidSocket = -1;
constexpr int kEmptyDescriptor = 0;

struct Request {
    std::string method="";
    std::string path="";
    std::string version="";
    std::map<std::string, std::string> headers;
    std::string body="";
};

struct Response {
    std::string version="";
    logrr::StatusCode status;
    std::map<std::string, std::string> headers;
    std::string body="";
};

struct ClientConnection {
    int sockfd = kInvalidSocket;
    std::string ip = "";
    uint16_t port = 0;
    std::string user_agent="";
};

class Server {
public:
    Server();
    virtual ~Server();
    Server(Server&& serv) noexcept;
    Server(const Server&) = delete;
    Server& operator=(Server&& serv) noexcept;
    Server& operator=(const Server&) = delete;

    bool listen();
    bool listen(const char* host, const char* port=DEF_HTTP_PORT, int max_connections=DEF_MAX_CONNECTIONS, int bufsize=DEF_RECEPTION_BUF_SIZE);
    bool continueListen();
    void stopListen() noexcept;
    bool addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept;
protected:
    bool buildSocket(const char* host, const char* port) noexcept;
    void freeAddrInfo(addrinfo*& servinfo) noexcept;
    void closeSocket(int& sockfd) noexcept;
    void closeConectToClient(ClientConnection& client) noexcept;
    template <typename... Opts> bool setSockOptions(int sockfd, Opts&&... args) noexcept;
    template <typename Opt> bool applyOption(int sockfd, Opt&& arg, int opt) noexcept;
    bool listenInternal(int max_connections, int bufsize);
    ClientConnection acceptConnection();
    void moveImpl(Server& serv) noexcept;
    std::string getRawReq(ClientConnection& client, int bufsize);
    Request parseReq(ClientConnection& client, std::string& req);
    // Response createResp(ClientConnection& client, Request& req);
    // bool sendResp(ClientConnection& client, Response& resp);
private:
    int sockfd_ = kEmptyDescriptor;
    addrinfo* servinfo_ = nullptr;
    addrinfo hints_;
    logrr::Logger logger_;
    bool is_running_ = false;
    std::mutex mtx_;
};

inline bool Server::listen() { return listen("0.0.0.0"); }
inline void Server::stopListen() noexcept { closeSocket(sockfd_); }

} // namespace http

#endif