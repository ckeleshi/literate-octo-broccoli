cmake_minimum_required(VERSION 3.21)

project(playground)

find_package(imgui CONFIG REQUIRED)

add_executable(playground
    main.cpp
)

target_link_libraries(playground PRIVATE imgui::imgui)
target_link_libraries(playground PRIVATE d3d9)
