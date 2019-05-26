cmake_minimum_required(VERSION 3.12)
project(OpenOBLBullet)

add_library(OpenOBLBullet STATIC)
add_library(OpenOBL::OpenOBLBullet ALIAS OpenOBLBullet)

target_include_directories(OpenOBLBullet PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLBullet PRIVATE
        ${CMAKE_SOURCE_DIR}/include/bullet/collision.hpp
        ${CMAKE_SOURCE_DIR}/include/bullet/configuration.hpp
        collision.cpp)

target_link_libraries(OpenOBLBullet PRIVATE
        OpenOBL::OpenOBLRecord
        OpenOBL::OpenOBLUtil
        spdlog::spdlog
        PUBLIC
        OpenOBL::OpenOBLNif
        OpenOBL::OpenOBLOgreBullet
        Bullet::BulletDynamics
        MicrosoftGSL::GSL)

install(TARGETS OpenOBLBullet EXPORT OpenOBLBulletTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLBulletTargets
        FILE OpenOBLBulletTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/bullet
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/bullet)
