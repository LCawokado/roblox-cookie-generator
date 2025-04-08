cmake_minimum_required(VERSION 3.30)
project(untitled)

set(CMAKE_CXX_STANDARD 20)
find_package(CURL REQUIRED)
add_executable(untitled main.cpp)
target_link_libraries(untitled PRIVATE CURL::libcurl)