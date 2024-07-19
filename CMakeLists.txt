cmake_minimum_required(VERSION 3.25)

project(grass-rendering)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.h"
)

add_executable(grass-rendering ${SOURCES})

target_include_directories(grass-rendering
    PUBLIC
    src
    third_party/glitch/include
)

target_link_directories(grass-rendering
    PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/glitch
)

target_link_libraries(grass-rendering
    PUBLIC
    glitch
)

target_precompile_headers(grass-rendering
    PUBLIC
    third_party/glitch/include/pch.h
)

