cmake_minimum_required(VERSION 3.19)
project(Linux_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(lib)

add_executable(Server
        lib/wrap.cpp
        lib/utils.cpp
        Chapter11/11_3/11_3.cpp)

#add_executable(Client
#        lib/wrap.cpp
#        lib/utils.cpp
#        Chapter11/11_1/11_1.cpp)

#target_link_libraries(Client pthread)
target_link_libraries(Server pthread)
