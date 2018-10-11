cmake_minimum_required(VERSION 3.12)
project(OpenOblivionSDL)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionSDL STATIC)

add_library(OpenOblivion::OpenOblivionSDL ALIAS OpenOblivionSDL)

target_include_directories(OpenOblivionSDL PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionSDL PRIVATE
        ${CMAKE_SOURCE_DIR}/include/sdl/sdl.hpp
        sdl.cpp)

target_link_libraries(OpenOblivionSDL PRIVATE Boost::boost
        PUBLIC SDL2::SDL2)

install(TARGETS OpenOblivionSDL EXPORT OpenOblivionSDLTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionSDLTargets
        FILE OpenOblivionSDLTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
