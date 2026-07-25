#include "http_server.h"

/*
    Example
*/

int main() {    
    http::Server server;
    server.addSink(std::make_shared<logrr::FileSink>("server.log"));
    server.listen();
}   