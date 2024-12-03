cmake_minimum_required(VERSION 3.31)
project(doshell)

set(CMAKE_CXX_STANDARD 14)

add_executable(doshell main.cpp
        shell.cpp)
