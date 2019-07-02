cmake_minimum_required(VERSION 3.12)
project(OpenOBLNifloader)

add_library(OpenOBLNifloader STATIC)
add_library(OpenOBL::OpenOBLNifloader ALIAS OpenOBLNifloader)

target_include_directories(OpenOBLNifloader PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_include_directories(OpenOBLNifloader PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_sources(OpenOBLNifloader PRIVATE
        ${CMAKE_SOURCE_DIR}/include/nifloader/animation.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/collision_object_loader.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/loader.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/logging.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/mesh_loader.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/nif_resource.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/nif_resource_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/scene.hpp
        ${CMAKE_SOURCE_DIR}/include/nifloader/skeleton_loader.hpp
        animation.cpp
        collision_object_loader.cpp
        collision_object_loader_state.cpp
        collision_object_loader_state.hpp
        loader.cpp
        mesh_loader.cpp
        mesh_loader_state.cpp
        mesh_loader_state.hpp
        nif_resource.cpp
        nif_resource_manager.cpp
        scene.cpp
        skeleton_loader.cpp
        skeleton_loader_state.cpp
        skeleton_loader_state.hpp)

target_link_libraries(OpenOBLNifloader
        PRIVATE
        OpenOBL::OpenOBLFS
        OpenOBL::OpenOBLMath
        PUBLIC
        OpenOBL::OpenOBLMesh
        OpenOBL::OpenOBLNif
        OpenOBL::OpenOBLUtil
        OpenOBL::OpenOBLOgreBullet
        Boost::boost
        MicrosoftGSL::GSL
        jbcoe::polymorphic_value
        spdlog::spdlog)

install(TARGETS OpenOBLNifloader EXPORT OpenOBLNifloaderTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLNifloaderTargets
        FILE OpenOBLNifTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/nifloader/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/nifloader)