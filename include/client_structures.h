#ifndef CLIENT_STRUCTURES
#define CLIENT_STRUCTURES

#include <random>
#include <sstream>
#include <iomanip>
#include <map>

#include "status.h"

#define DEF_HTTP_PORT "8080"
#define DEF_MAX_CONNECTIONS 20
#define DEF_RECEPTION_BUF_SIZE 32768

namespace http {

std::string generate_uuid_v4();

constexpr int kInvalidSocket = -1;
constexpr int kEmptyDescriptor = 0;

struct Request 
{
    std::string method="";
    std::string path="";
    std::string version="";
    std::map<std::string, std::string> headers;
    std::string body="";
};

struct Response 
{
    std::string version="";
    status status;
    std::map<std::string, std::string> headers;
    std::string body="";
};

struct ClientConnection 
{
    std::string id;
    int sockfd = kInvalidSocket;
    std::string ip = "";
    uint16_t port = 0;
};

} // namespace http

#endif 