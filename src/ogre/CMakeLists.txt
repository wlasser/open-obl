cmake_minimum_required(VERSION 3.12)
project(OpenOblivionOgre)

add_library(OpenOblivionOgre STATIC)
add_library(OpenOblivion::OpenOblivionOgre ALIAS OpenOblivionOgre)

target_include_directories(OpenOblivionOgre PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionOgre PRIVATE
        ${CMAKE_SOURCE_DIR}/include/ogre/fnt_loader.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/ogre_stream_wrappers.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/tex_image_codec.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/text_resource.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/text_resource_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/spdlog_listener.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/window.hpp
        fnt_loader.cpp
        tex_image_codec.cpp
        text_resource.cpp
        text_resource_manager.cpp
        ogre_stream_wrappers.cpp
        window.cpp)

target_link_libraries(OpenOblivionOgre
        PRIVATE spdlog OpenOblivion::OpenOblivionFS
        PUBLIC OgreMain OgreOverlay)

install(TARGETS OpenOblivionOgre EXPORT OpenOblivionOgreTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionOgreTargets
        FILE OpenOblivionOgreTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
