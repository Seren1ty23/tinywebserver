cmake_minimum_required(VERSION 3.16)
project(tinywebserver)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

add_executable(tinywebserver
        main.cpp)

add_executable(io_demo
        test/io_demo.cpp)

add_executable(lock_test
        test/lock_test.cpp
        lock/locker.h)