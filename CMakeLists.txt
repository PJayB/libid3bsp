cmake_minimum_required(VERSION 3.1)

project(libid3bsp)

add_subdirectory(lexy)

add_library(libid3bsp
    src/BSP.cpp
    src/Entities.cpp
    )

target_compile_features(libid3bsp PRIVATE cxx_std_17)

target_include_directories(libid3bsp
    PUBLIC include
    PRIVATE lexy/include)

target_link_libraries(libid3bsp PRIVATE lexy)
