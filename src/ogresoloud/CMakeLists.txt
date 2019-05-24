cmake_minimum_required(VERSION 3.12)
project(OpenOBLOgreSoLoud)

add_library(OpenOBLOgreSoLoud STATIC)
add_library(OpenOBL::OpenOBLOgreSoLoud ALIAS OpenOBLOgreSoLoud)

target_include_directories(OpenOBLOgreSoLoud PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLOgreSoLoud PRIVATE
        ${CMAKE_SOURCE_DIR}/include/ogresoloud/sound_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogresoloud/wav_resource.hpp
        ${CMAKE_SOURCE_DIR}/include/ogresoloud/wav_resource_manager.hpp
        sound_manager.cpp
        wav_resource.cpp
        wav_resource_manager.cpp)

target_link_libraries(OpenOBLOgreSoLoud PUBLIC
        SoLoud::SoLoud
        OgreMain)

install(TARGETS OpenOBLOgreSoLoud EXPORT OpenOBLOgreSoLoudTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLOgreSoLoudTargets
        FILE OpenOBLOgreSoLoudTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ogresoloud/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/ogresoloud)