cmake_minimum_required(VERSION 3.12)
project(OpenOBLApplicationBase)

add_library(OpenOBLApplicationBase STATIC)
add_library(OpenOBL::OpenOBLApplicationBase ALIAS OpenOBLApplicationBase)

target_include_directories(OpenOBLApplicationBase PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLApplicationBase PRIVATE
        ${CMAKE_SOURCE_DIR}/include/application_base/application_base.hpp
        application_base.cpp)

target_link_libraries(OpenOBLApplicationBase PRIVATE
        OpenOBL::OpenOBLUtil
        spdlog::spdlog
        PUBLIC
        OpenOBL::OpenOBLOgre
        OpenOBL::OpenOBLSDL
        RenderSystem_GL3Plus)

install(TARGETS OpenOBLApplicationBase EXPORT OpenOBLApplicationBaseTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLApplicationBaseTargets
        FILE OpenOBLApplicationBaseTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/application_base
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/application_base)
