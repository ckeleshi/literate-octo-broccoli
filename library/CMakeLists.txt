cmake_minimum_required(VERSION 3.21)

project(library)

find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

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
target_link_libraries(library PRIVATE spdlog::spdlog)

set_target_properties(library PROPERTIES
    OUTPUT_NAME "opengl32"
)
