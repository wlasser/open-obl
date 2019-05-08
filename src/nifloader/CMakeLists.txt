cmake_minimum_required(VERSION 3.12)
project(OpenOblivionNifloader)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionNifloader STATIC)
add_library(OpenOblivion::OpenOblivionNifloader ALIAS OpenOblivionNifloader)

target_compile_options(OpenOblivionNifloader PRIVATE -Wall -Wextra)
target_compile_features(OpenOblivionNifloader PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionNifloader PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOblivionNifloader PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionNifloader PRIVATE
        ${CMAKE_SOURCE_DIR}/include/nifloader/animation.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/collision_object_loader.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/collision_object_loader_state.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/loader.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/logging.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/mesh_loader.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/mesh_loader_state.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/nif_resource.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/nif_resource_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/scene.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/skeleton_loader.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/skeleton_loader_state.hpp
        animation.cpp
        collision_object_loader.cpp
        collision_object_loader_state.cpp
        loader.cpp
        mesh_loader.cpp
        mesh_loader_state.cpp
        nif_resource.cpp
        nif_resource_manager.cpp
        scene.cpp
        skeleton_loader.cpp
        skeleton_loader_state.cpp)

target_link_libraries(OpenOblivionNifloader
        PRIVATE
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionMath
        PUBLIC
        OpenOblivion::OpenOblivionMesh
        OpenOblivion::OpenOblivionNif
        OpenOblivion::OpenOblivionOgreBullet
        Boost::boost
        GSL
        polymorphic_value
        spdlog::spdlog)

install(TARGETS OpenOblivionNifloader EXPORT OpenOblivionNifloaderTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionNifloaderTargets
        FILE OpenOblivionNifTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)