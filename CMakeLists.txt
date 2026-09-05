cmake_minimum_required(VERSION 3.16)
project(tinywebserver)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

add_executable(tinywebserver
        main.cpp
        gdb_demo.cpp)
