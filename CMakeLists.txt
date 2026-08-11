cmake_minimum_required(VERSION 3.16)

project(torus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(torus
    src/main.cpp
)

target_compile_options(torus PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(torus SYSTEM PRIVATE /opt/homebrew/include)
