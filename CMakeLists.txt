cmake_minimum_required(VERSION 3.28.0)

project(zoro)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

include_directories(include)
add_subdirectory(src)