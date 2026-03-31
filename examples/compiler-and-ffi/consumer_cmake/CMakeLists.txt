cmake_minimum_required(VERSION 3.16)
project(t81_external_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(T81Foundation CONFIG REQUIRED)

add_executable(t81_consumer main.cpp)
target_link_libraries(t81_consumer PRIVATE T81::t81_core)
