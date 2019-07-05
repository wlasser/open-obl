cmake_minimum_required(VERSION 3.12)
project(OpenOBLBsa)

add_library(OpenOBLBsa STATIC)
add_library(OpenOBL::OpenOBLBsa ALIAS OpenOBLBsa)

target_include_directories(OpenOBLBsa PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLBsa PRIVATE
        ${CMAKE_SOURCE_DIR}/include/bsa/bsa.hpp
        bsa.cpp)

target_link_libraries(OpenOBLBsa PRIVATE
        OpenOBL::OpenOBLUtil
        ZLIB::ZLIB
        PUBLIC
        OpenOBL::OpenOBLIO
        Boost::fiber)

install(TARGETS OpenOBLBsa EXPORT OpenOBLBsaTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLBsaTargets
        FILE OpenOBLBsaTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/bsa/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/bsa)