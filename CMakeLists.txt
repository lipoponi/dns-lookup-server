cmake_minimum_required(VERSION 3.15)
project(dns_lookup_server)

set(CMAKE_CXX_STANDARD 17)

set(REQ_FILES
        base_server.cpp
        gai_server.cpp)

add_executable(dns-lookup-server main.cpp ${REQ_FILES})
