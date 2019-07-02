cmake_minimum_required(VERSION 3.12)
project(OpenOBLSDL)

add_library(OpenOBLSDL STATIC)
add_library(OpenOBL::OpenOBLSDL ALIAS OpenOBLSDL)

target_include_directories(OpenOBLSDL PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLSDL PRIVATE
        ${CMAKE_SOURCE_DIR}/include/sdl/sdl.hpp
        sdl.cpp)

target_link_libraries(OpenOBLSDL PUBLIC OpenOBL::OpenOBLUtil SDL2::SDL2)

install(TARGETS OpenOBLSDL EXPORT OpenOBLSDLTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLSDLTargets
        FILE OpenOBLSDLTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/sdl/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/sdl)