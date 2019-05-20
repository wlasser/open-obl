cmake_minimum_required(VERSION 3.12)
project(OpenOblivionNif)

add_library(OpenOblivionNif STATIC)
add_library(OpenOblivion::OpenOblivionNif ALIAS OpenOblivionNif)

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

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/nif/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOblivion/nif)