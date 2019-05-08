cmake_minimum_required(VERSION 3.12)
project(OpenOblivionIO)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionIO STATIC)
add_library(OpenOblivion::OpenOblivionIO ALIAS OpenOblivionIO)

target_compile_options(OpenOblivionIO PRIVATE -Wall -Wextra)
target_compile_features(OpenOblivionIO PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionIO PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOblivionIO PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionIO PRIVATE
        ${CMAKE_SOURCE_DIR}/include/io/io.hpp
        ${CMAKE_SOURCE_DIR}/include/io/memstream.hpp
        ${CMAKE_SOURCE_DIR}/include/io/string.hpp
        io.cpp
        string.cpp)

target_link_libraries(OpenOblivionIO PRIVATE Boost::boost)

install(TARGETS OpenOblivionIO EXPORT OpenOblivionIOTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionIOTargets
        FILE OpenOblivionIOTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
