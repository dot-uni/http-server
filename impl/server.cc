#include "http_server.h"

/*
    Example
*/


int main() {
    http::HttpServer server;
    server.addSink(std::make_shared<logrr::FileSink>());
    server.listen();
}   