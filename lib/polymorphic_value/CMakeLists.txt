cmake_minimum_required(VERSION 3.12)

project(polymorphic_value CXX)
set(CMAKE_CXX_STANDARD 14)

add_library(polymorphic_value INTERFACE)

target_include_directories(polymorphic_value INTERFACE
        $<BUILD_INTERFACE: ${CMAKE_CURRENT_SOURCE_DIR}/include)
