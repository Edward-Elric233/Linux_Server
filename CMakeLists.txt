cmake_minimum_required(VERSION 3.19)
project(Linux_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(lib)

add_executable(Server
        lib/wrap.cpp
        lib/utils.cpp
        Chapter10/10_1/10_1.cpp)

target_link_libraries(Server pthread)
