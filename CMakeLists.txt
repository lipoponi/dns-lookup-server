cmake_minimum_required(VERSION 3.15)
project(dns_lookup_server)

set(CMAKE_CXX_STANDARD 17)

add_executable(dns_lookup_server main.cpp)
add_executable(scratch scratch.cpp)