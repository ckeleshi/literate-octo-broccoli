cmake_minimum_required(VERSION 3.21)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(ffo-ingame-helper)

include("${CMAKE_SOURCE_DIR}/cmake/VC-LTL helper for cmake.cmake")

find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(library SHARED
    ffo.cpp
    dll.cpp
    dll.def
    helper/helper.cpp
    hooking/byte_pattern.cpp
)

target_link_libraries(library PRIVATE imgui::imgui)
target_link_libraries(library PRIVATE winmm opengl32)
target_link_libraries(library PRIVATE nlohmann_json::nlohmann_json)

set_target_properties(library PROPERTIES
    OUTPUT_NAME "opengl32"
)
