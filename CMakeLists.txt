cmake_minimum_required(VERSION 3.4)
project(text_query)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(text_query ${SOURCE_FILES})