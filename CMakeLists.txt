cmake_minimum_required(VERSION 3.21)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(FFOHelper)

include("${CMAKE_SOURCE_DIR}/cmake/VC-LTL helper for cmake.cmake")

add_subdirectory(library)
add_subdirectory(playground)
