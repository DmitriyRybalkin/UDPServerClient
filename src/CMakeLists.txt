cmake_minimum_required(VERSION 2.8)

PROJECT(UDPClientServer C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory (UDPServer)
add_subdirectory (UDPClient)