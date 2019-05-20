cmake_minimum_required(VERSION 3.12)
project(OpenOblivionOgreSoLoud)

add_library(OpenOblivionOgreSoLoud STATIC)
add_library(OpenOblivion::OpenOblivionOgreSoLoud ALIAS OpenOblivionOgreSoLoud)

target_include_directories(OpenOblivionOgreSoLoud PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionOgreSoLoud PRIVATE
        ${CMAKE_SOURCE_DIR}/include/ogresoloud/sound_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogresoloud/wav_resource.hpp
        ${CMAKE_SOURCE_DIR}/include/ogresoloud/wav_resource_manager.hpp
        sound_manager.cpp
        wav_resource.cpp
        wav_resource_manager.cpp)

target_link_libraries(OpenOblivionOgreSoLoud PUBLIC
        SoLoud::SoLoud
        OgreMain)

install(TARGETS OpenOblivionOgreSoLoud EXPORT OpenOblivionOgreSoLoudTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionOgreSoLoudTargets
        FILE OpenOblivionOgreSoLoudTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ogresoloud/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOblivion/ogresoloud)