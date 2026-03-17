cmake_minimum_required(VERSION 3.10)

project(player VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_BUILD_TYPE Debug)

# 添加子目录
add_subdirectory(src)
