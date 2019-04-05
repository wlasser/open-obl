cmake_minimum_required(VERSION 3.12)

project(polymorphic_value CXX)
set(CMAKE_CXX_STANDARD 14)

add_library(polymorphic_value INTERFACE)

target_include_directories(polymorphic_value INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

install(TARGETS polymorphic_value EXPORT polymorphic_valueTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT polymorphic_valueTargets
        FILE polymorphic_valueTargets.cmake
        NAMESPACE jbcoe::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jbcoe)
