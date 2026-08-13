cmake_minimum_required(VERSION 3.16)

project(torus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)

add_executable(torus
    src/benchmark.cpp
)

target_compile_options(torus PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(torus PRIVATE Boost::boost)

find_package(Threads REQUIRED)
target_link_libraries(torus PRIVATE Threads::Threads)
