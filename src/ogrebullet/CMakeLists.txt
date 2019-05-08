cmake_minimum_required(VERSION 3.12)
project(OpenOblivionOgreBullet)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionOgreBullet STATIC)
add_library(OpenOblivion::OpenOblivionOgreBullet ALIAS OpenOblivionOgreBullet)

target_compile_options(OpenOblivionOgreBullet PRIVATE -Wall -Wextra)
target_compile_features(OpenOblivionOgreBullet PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionOgreBullet PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOblivionOgreBullet PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PUBLIC
        ${BULLET_INCLUDE_DIRS})

target_sources(OpenOblivionOgreBullet PRIVATE
        ${CMAKE_SOURCE_DIR}/include/ogrebullet/collision_shape.hpp
        ${CMAKE_SOURCE_DIR}/include/ogrebullet/collision_shape_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogrebullet/conversions.hpp
        ${CMAKE_SOURCE_DIR}/include/ogrebullet/debug_drawer.hpp
        ${CMAKE_SOURCE_DIR}/include/ogrebullet/motion_state.hpp
        ${CMAKE_SOURCE_DIR}/include/ogrebullet/rigid_body.hpp
        collision_shape.cpp
        collision_shape_manager.cpp
        debug_drawer.cpp
        motion_state.cpp
        rigid_body.cpp)

target_link_libraries(OpenOblivionOgreBullet PRIVATE
        OpenOblivion::OpenOblivionNif
        OpenOblivion::OpenOblivionOgre
        spdlog
        PUBLIC ${BULLET_LIBRARY} GSL OgreMain)

install(TARGETS OpenOblivionOgreBullet EXPORT OpenOblivionOgreBulletTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionOgreBulletTargets
        FILE OpenOblivionOgreBulletTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
