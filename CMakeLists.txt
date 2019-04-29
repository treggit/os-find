cmake_minimum_required(VERSION 3.0)
project(os_find)

set(CMAKE_CXX_STANDARD 17)

add_executable(find linux_dirent.h file_descriptor.h find_config.h main.cpp)