cmake_minimum_required(VERSION 3.12)
project(OpenOBLConfig)

add_library(OpenOBLConfig STATIC)
add_library(OpenOBL::OpenOBLConfig ALIAS OpenOBLConfig)

target_include_directories(OpenOBLConfig PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLConfig PRIVATE
        ${CMAKE_SOURCE_DIR}/include/config/game_settings.hpp
        ${CMAKE_SOURCE_DIR}/include/config/globals.hpp
        game_settings.cpp
        globals.cpp)

target_link_libraries(OpenOBLConfig PRIVATE
        OpenOBL::OpenOBLUtil
        spdlog::spdlog
        PUBLIC
        OpenOBL::OpenOBLFS
        OpenOBL::OpenOBLRecord
        Boost::boost
        Boost::fiber
        tl::tloptional)

install(TARGETS OpenOBLConfig EXPORT OpenOBLConfigTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLConfigTargets
        FILE OpenOBLConfigTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/esp/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/config)
