cmake_minimum_required(VERSION 3.12)
project(OpenOBLEsp)

add_library(OpenOBLEsp STATIC)
add_library(OpenOBL::OpenOBLEsp ALIAS OpenOBLEsp)

target_include_directories(OpenOBLEsp PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLEsp PRIVATE
        ${CMAKE_SOURCE_DIR}/include/esp/esp.hpp
        ${CMAKE_SOURCE_DIR}/include/esp/esp_coordinator.hpp
        esp_coordinator.cpp)

target_link_libraries(OpenOBLEsp PUBLIC
        OpenOBL::OpenOBLFS
        OpenOBL::OpenOBLIO
        OpenOBL::OpenOBLRecord
        OpenOBL::OpenOBLUtil
        Boost::fiber
        MicrosoftGSL::GSL
        spdlog::spdlog)

install(TARGETS OpenOBLEsp EXPORT OpenOBLEspTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLEspTargets
        FILE OpenOBLEspTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/esp/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/esp)
