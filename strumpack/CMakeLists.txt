cmake_minimum_required(VERSION 3.5)
project(strumpack LANGUAGES CXX)
 
find_package(STRUMPACK REQUIRED)
 
add_executable(exe main.cpp)
target_link_libraries(exe PRIVATE STRUMPACK::strumpack)
