cmake_minimum_required(VERSION 3.12)
project(OpenOblivionOgre)

add_library(OpenOblivionOgre STATIC)
add_library(OpenOblivion::OpenOblivionOgre ALIAS OpenOblivionOgre)

target_include_directories(OpenOblivionOgre PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionOgre PRIVATE
        ${CMAKE_SOURCE_DIR}/include/ogre/bsa_archive_factory.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/deferred_light_pass.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/fnt_loader.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/interior_scene_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/ogre_stream_wrappers.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/scene_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/spdlog_listener.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/terrain_material_generator.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/tex_image_codec.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/text_resource.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/text_resource_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogre/window.hpp
        bsa_archive_factory.cpp
        deferred_light_pass.cpp
        fnt_loader.cpp
        interior_scene_manager.cpp
        ogre_stream_wrappers.cpp
        scene_manager.cpp
        terrain_material_generator.cpp
        tex_image_codec.cpp
        text_resource.cpp
        text_resource_manager.cpp
        window.cpp)

target_link_libraries(OpenOblivionOgre PRIVATE
        spdlog::spdlog
        OpenOblivion::OpenOblivionBsa
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionMath
        PUBLIC
        Boost::boost
        MicrosoftGSL::GSL
        OgreMain
        OgreOverlay)

install(TARGETS OpenOblivionOgre EXPORT OpenOblivionOgreTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionOgreTargets
        FILE OpenOblivionOgreTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ogre/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOblivion/ogre)