cmake_minimum_required(VERSION 3.12)
project(OpenOBLNif)

add_library(OpenOBLNif STATIC)
add_library(OpenOBL::OpenOBLNif ALIAS OpenOBLNif)

target_include_directories(OpenOBLNif PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLNif PRIVATE
        ${CMAKE_SOURCE_DIR}/include/nif/basic.hpp
        ${CMAKE_SOURCE_DIR}/include/nif/compound.hpp
        ${CMAKE_SOURCE_DIR}/include/nif/enum.hpp
        ${CMAKE_SOURCE_DIR}/include/nif/niobject.hpp
        ${CMAKE_SOURCE_DIR}/include/nif/versionable.hpp
        basic.cpp
        compound.cpp
        niobject.cpp)

target_link_libraries(OpenOBLNif PRIVATE
        Boost::boost
        OpenOBL::OpenOBLIO)

install(TARGETS OpenOBLNif EXPORT OpenOBLNifTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLNifTargets
        FILE OpenOBLNifTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/nif/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/nif)