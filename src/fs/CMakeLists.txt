cmake_minimum_required(VERSION 3.12)
project(OpenOblivionFS)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionFS STATIC)

add_library(OpenOblivion::OpenOblivionFS ALIAS OpenOblivionFS)

target_include_directories(OpenOblivionFS PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionFS PRIVATE
        ${CMAKE_SOURCE_DIR}/include/fs/path.hpp
        path.cpp)

target_link_libraries(OpenOblivionFS PRIVATE
        Boost::boost
        stdc++fs)

install(TARGETS OpenOblivionFS EXPORT OpenOblivionFSTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionFSTargets
        FILE OpenOblivionFSTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
