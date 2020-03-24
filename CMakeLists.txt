cmake_minimum_required(VERSION 3.10)
project(dns_lookup_server)

set(CMAKE_CXX_STANDARD 17)

IF (CMAKE_BUILD_TYPE)
  string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UP)
ENDIF ()
IF (CMAKE_BUILD_TYPE_UP STREQUAL "DEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
ENDIF ()

set(REQ_FILES
        address.cpp
        app.cpp
        endpoint.cpp
        logger.cpp
        unique_fd.cpp)
add_executable(dns-lookup-server main.cpp ${REQ_FILES})

target_link_libraries(dns-lookup-server pthread)
