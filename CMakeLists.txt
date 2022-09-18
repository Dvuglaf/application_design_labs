cmake_minimum_required(VERSION 3.5)
project(Design CXX)

set(CMAKE_CXX_STANDARD 20)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_compile_options(/W4 /D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(lab1_client_server)


