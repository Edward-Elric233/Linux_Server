cmake_minimum_required(VERSION 3.19)
project(Linux_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(lib)

add_executable(Linux_Server
        lib/wrap.cpp
        lib/utils.cpp
        Chapter5/5_5/5_5.cpp)

target_link_libraries(Linux_Server pthread)