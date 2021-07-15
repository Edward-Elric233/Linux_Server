cmake_minimum_required(VERSION 3.19)
project(Linux_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(lib)

#add_executable(Client
#        lib/wrap.cpp
#        lib/utils.cpp
#        Chapter5/5_6-7/5_6.cpp)

add_executable(Server
        lib/wrap.cpp
        lib/utils.cpp
        Chapter5/5_6-7/5_7.cpp)

#target_link_libraries(Client pthread)
target_link_libraries(Server pthread)
