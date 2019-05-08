cmake_minimum_required(VERSION 3.12)
project(OpenOblivionMesh)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionMesh STATIC)

add_library(OpenOblivion::OpenOblivionMesh ALIAS OpenOblivionMesh)

target_include_directories(OpenOblivionMesh PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionMesh PRIVATE
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

target_link_libraries(OpenOblivionMesh
        PRIVATE Boost::boost
        PUBLIC GSL OgreMain)

install(TARGETS OpenOblivionMesh EXPORT OpenOblivionMeshTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionMeshTargets
        FILE OpenOblivionMeshTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDDIR}/cmake/OpenOblivion)
