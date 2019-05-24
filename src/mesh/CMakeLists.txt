cmake_minimum_required(VERSION 3.12)
project(OpenOBLMesh)

add_library(OpenOBLMesh STATIC)
add_library(OpenOBL::OpenOBLMesh ALIAS OpenOBLMesh)

target_include_directories(OpenOBLMesh PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLMesh PRIVATE
        ${CMAKE_SOURCE_DIR}/include/mesh/entity.hpp
        ${CMAKE_SOURCE_DIR}/include/mesh/mesh.hpp
        ${CMAKE_SOURCE_DIR}/include/mesh/mesh_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/mesh/subentity.hpp
        ${CMAKE_SOURCE_DIR}/include/mesh/submesh.hpp
        entity.cpp
        mesh.cpp
        mesh_manager.cpp
        subentity.cpp
        submesh.cpp)

target_link_libraries(OpenOBLMesh
        PRIVATE Boost::boost
        PUBLIC MicrosoftGSL::GSL OgreMain)

install(TARGETS OpenOBLMesh EXPORT OpenOBLMeshTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLMeshTargets
        FILE OpenOBLMeshTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/mesh/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/mesh)