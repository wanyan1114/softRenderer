cmake_minimum_required(VERSION 3.26)

project(SoftRenderer
    VERSION 0.1.0
    DESCRIPTION "Software rasterizer skeleton on Windows"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(NOT WIN32)
    message(FATAL_ERROR "This skeleton currently targets Windows only.")
endif()

add_subdirectory(src)
add_subdirectory(apps/viewer)
