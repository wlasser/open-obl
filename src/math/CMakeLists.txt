cmake_minimum_required(VERSION 3.12)
project(OpenOblivionMath)

add_library(OpenOblivionMath INTERFACE)
add_library(OpenOblivion::OpenOblivionMath ALIAS OpenOblivionMath)

target_include_directories(OpenOblivionMath INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(OpenOblivionMath INTERFACE
        OpenOblivion::OpenOblivionNif
        Boost::boost
        Bullet::LinearMath
        OgreMain)

install(TARGETS OpenOblivionMath EXPORT OpenOblivionMathTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT OpenOblivionMathTargets
        FILE OpenOblivionMathTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/math/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOblivion/math)