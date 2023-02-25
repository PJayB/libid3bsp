cmake_minimum_required(VERSION 3.1)

project(libid3bsp)

add_subdirectory(lexy)

add_library(libid3bsp
    BSP.cpp
    )

target_include_directories(libid3bsp PRIVATE lexy/include)
target_link_libraries(libid3bsp PRIVATE lexy)
