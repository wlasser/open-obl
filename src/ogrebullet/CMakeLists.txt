cmake_minimum_required(VERSION 3.12)
project(OpenOBLOgreBullet)

add_library(OpenOBLOgreBullet STATIC)
add_library(OpenOBL::OpenOBLOgreBullet ALIAS OpenOBLOgreBullet)

target_include_directories(OpenOBLOgreBullet PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLOgreBullet PRIVATE
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

target_link_libraries(OpenOBLOgreBullet PRIVATE
        OpenOBL::OpenOBLNif
        OpenOBL::OpenOBLOgre
        spdlog::spdlog
        PUBLIC
        OpenOBL::OpenOBLUtil
        Bullet::BulletCollision
        Bullet::BulletDynamics
        MicrosoftGSL::GSL
        OgreMain)

install(TARGETS OpenOBLOgreBullet EXPORT OpenOBLOgreBulletTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLOgreBulletTargets
        FILE OpenOBLOgreBulletTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ogrebullet/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/ogrebullet)
