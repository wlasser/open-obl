cmake_minimum_required(VERSION 3.12)
project(OpenOBLMath)

add_library(OpenOBLMath INTERFACE)
add_library(OpenOBL::OpenOBLMath ALIAS OpenOBLMath)

target_include_directories(OpenOBLMath INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(OpenOBLMath INTERFACE
        OpenOBL::OpenOBLNif
        Boost::boost
        Bullet::LinearMath
        OgreMain)

install(TARGETS OpenOBLMath EXPORT OpenOBLMathTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT OpenOBLMathTargets
        FILE OpenOBLMathTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/math/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/math)