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
        ${CMAKE_SOURCE_DIR}/include/scripting/console_engine.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/grammar.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/jit.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/llvm.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/logging.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/pegtl.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/script_engine.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/script_engine_base.hpp
        ast.cpp
        console_engine.cpp
        grammar.cpp
        jit.cpp
        llvm.cpp
        script_engine.cpp
        script_engine_base.cpp)

target_include_directories(OpenOblivionScripting PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(OpenOblivionScripting PRIVATE ${LLVM_DEFINITIONS})

if (USE_SHARED_LLVM_LIBS)
    llvm_config(OpenOblivionScripting USE_SHARED engine)
else ()
    llvm_config(OpenOblivionScripting engine)
endif ()

target_link_libraries(OpenOblivionScripting
        PRIVATE taocpp::pegtl GSL
        PUBLIC ${llvm_libs} spdlog::spdlog)

set_target_properties(OpenOblivionScripting PROPERTIES
        CXX_VISIBILITY_PRESET hidden)
target_link_libraries(OpenOblivionScripting PRIVATE "-Wl,--export-dynamic")

install(TARGETS OpenOblivionScripting EXPORT OpenOblivionScriptingTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionScriptingTargets
        FILE OpenOblivionScriptingTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
