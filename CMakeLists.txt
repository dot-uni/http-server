cmake_minimum_required(VERSION 3.20)
project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt CONFIG REQUIRED)

add_executable(server server.cc logging.cc http_server.cc)
target_link_libraries(server PRIVATE fmt::fmt)