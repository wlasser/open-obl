cmake_minimum_required(VERSION 3.12)
project(OpenOblivionNif)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionNif STATIC)
add_library(OpenOblivion::OpenOblivionNif ALIAS OpenOblivionNif)

target_compile_options(OpenOblivionNif PRIVATE -Wall -Wextra)
target_compile_features(OpenOblivionNif PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionNif PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOblivionNif PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionNif PRIVATE
        ${CMAKE_SOURCE_DIR}/include/nif/basic.hpp
        ${CMAKE_SOURCE_DIR}/include/nif/compound.hpp
        ${CMAKE_SOURCE_DIR}/include/nif/enum.hpp
        ${CMAKE_SOURCE_DIR}/include/nif/niobject.hpp
        ${CMAKE_SOURCE_DIR}/include/nif/versionable.hpp
        basic.cpp
        compound.cpp
        niobject.cpp)

target_link_libraries(OpenOblivionNif PRIVATE
        Boost::boost
        OpenOblivion::OpenOblivionIO)

install(TARGETS OpenOblivionNif EXPORT OpenOblivionNifTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionNifTargets
        FILE OpenOblivionNifTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
