cmake_minimum_required(VERSION 3.12)
project(OpenOblivionMath)

add_library(OpenOblivionMath INTERFACE)
add_library(OpenOblivion::OpenOblivionMath ALIAS OpenOblivionMath)

target_include_directories(OpenOblivionMath INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${BULLET_INCLUDE_DIRS})

target_link_libraries(OpenOblivionMath INTERFACE
        OpenOblivion::OpenOblivionNif
        Boost::boost
        ${BULLET_LIBRARY}
        OgreMain)

install(TARGETS OpenOblivionMath EXPORT OpenOblivionMathTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionMathTargets
        FILE OpenOblivionMathTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOblivion/math)