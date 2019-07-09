cmake_minimum_required(VERSION 3.12)
project(OpenOBLJob)

add_library(OpenOBLJob INTERFACE)
add_library(OpenOBL::OpenOBLJob ALIAS OpenOBLJob)

target_include_directories(OpenOBLJob INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(OpenOBLJob INTERFACE Boost::fiber)

install(TARGETS OpenOBLJob EXPORT OpenOBLJobTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT OpenOBLJobTargets
        FILE OpenOBLJobTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/job/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/job)