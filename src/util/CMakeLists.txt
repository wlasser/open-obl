cmake_minimum_required(VERSION 3.12)
project(OpenOBLUtil)

add_library(OpenOBLUtil INTERFACE)
add_library(OpenOBL::OpenOBLUtil ALIAS OpenOBLUtil)

target_include_directories(OpenOBLUtil INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

install(TARGETS OpenOBLUtil EXPORT OpenOBLUtilTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT OpenOBLUtilTargets
        FILE OpenOBLUtilTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/util/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/util)