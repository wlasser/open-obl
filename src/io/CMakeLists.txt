cmake_minimum_required(VERSION 3.12)
project(OpenOBLIO)

add_library(OpenOBLIO STATIC)
add_library(OpenOBL::OpenOBLIO ALIAS OpenOBLIO)

target_include_directories(OpenOBLIO PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLIO PRIVATE
        ${CMAKE_SOURCE_DIR}/include/io/io.hpp
        ${CMAKE_SOURCE_DIR}/include/io/memstream.hpp
        ${CMAKE_SOURCE_DIR}/include/io/string.hpp
        io.cpp
        string.cpp)

target_link_libraries(OpenOBLIO
        PUBLIC OpenOBL::OpenOBLUtil
        PRIVATE Boost::boost)

install(TARGETS OpenOBLIO EXPORT OpenOBLIOTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLIOTargets
        FILE OpenOBLIOTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/io/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/io)