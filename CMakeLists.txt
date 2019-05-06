cmake_minimum_required(VERSION 3.0)

project(wslaypp VERSION 0.0.1 DESCRIPTION "wslay wrapper in C++")

file(GLOB wslaypp_sources
    "lib/*.c"
    "lib/*.h"
    "lib/*.hpp"
    "lib/*.cpp"
)

set(PUBLIC_HEADERS lib/includes/wslay/wslay.h lib/includes/wslaypp/wslaypp.hpp)

add_library(wslaypp SHARED ${wslaypp_sources})

set_target_properties(wslaypp PROPERTIES
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

target_include_directories(wslaypp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/includes)

install(
    TARGETS wslaypp
    LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/
    PUBLIC_HEADER DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/include/
)