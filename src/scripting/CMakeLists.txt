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
        ${CMAKE_SOURCE_DIR}/include/scripting/ast.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/grammar.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/jit.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/llvm.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/pegtl.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/script_engine.hpp
        ast.cpp
        grammar.cpp
        jit.cpp
        llvm.cpp
        script_engine.cpp)

target_include_directories(OpenOblivionScripting PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(OpenOblivionScripting PUBLIC ${LLVM_DEFINITIONS})
# TODO: Choose this more carefully out of llvm-config --components
llvm_map_components_to_libnames(llvm_libs all)

target_link_libraries(OpenOblivionScripting PUBLIC taocpp::pegtl)

set_target_properties(OpenOblivionScripting PROPERTIES
        CXX_VISIBILITY_PRESET hidden)
target_link_libraries(OpenOblivionScripting PRIVATE "-Wl,--export-dynamic")

install(TARGETS OpenOblivionScripting EXPORT OpenOblivionScriptingTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionScriptingTargets
        FILE OpenOblivionScriptingTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
