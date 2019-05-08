cmake_minimum_required(VERSION 3.12)
project(OpenOblivionBsa)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionBsa STATIC)
add_library(OpenOblivion::OpenOblivionBsa ALIAS OpenOblivionBsa)

target_compile_options(OpenOblivionBsa PRIVATE -Wall -Wextra)
target_compile_features(OpenOblivionBsa PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionBsa PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOblivionBsa PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionBsa PRIVATE
        ${CMAKE_SOURCE_DIR}/include/bsa/bsa.hpp
        bsa.cpp)

target_link_libraries(OpenOblivionBsa PRIVATE Boost::boost ZLIB::ZLIB
        PUBLIC OpenOblivion::OpenOblivionIO)

install(TARGETS OpenOblivionBsa EXPORT OpenOblivionBsaTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionBsaTargets
        FILE OpenOblivionBsaTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)