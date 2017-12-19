cmake_minimum_required(VERSION 2.8.7)
project(kcp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(MAIN_TEST kcp_client.cpp)
set(SERV_TEST kcp_server.cpp)

set(SOURCE_FILES ikcp.c sess.cpp)
add_executable(kcp_client ${SOURCE_FILES} ${MAIN_TEST})
add_executable(kcp_server ${SOURCE_FILES} ${SERV_TEST})

if(UNIX)
target_link_libraries(kcp_client pthread rt)
target_link_libraries(kcp_server pthread rt)
endif(UNIX)
