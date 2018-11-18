cmake_minimum_required(VERSION 3.12)
project(OpenOblivionScripting)

add_library(OpenOblivionScripting STATIC)
target_compile_options(OpenOblivionScripting PRIVATE -Wall)
target_compile_features(OpenOblivionScripting PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionScripting PROPERTY CXX_EXTENSION OFF)

add_library(OpenOblivion::OpenOblivionScripting ALIAS OpenOblivionScripting)

target_include_directories(OpenOblivionScripting PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionScripting PRIVATE
        ${CMAKE_SOURCE_DIR}/include/scripting/grammar.hpp
        grammar.cpp)

target_link_libraries(OpenOblivionScripting PUBLIC taocpp::pegtl)

install(TARGETS OpenOblivionScripting EXPORT OpenOblivionScriptingTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionScriptingTargets
        FILE OpenOblivionScriptingTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
