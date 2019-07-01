cmake_minimum_required(VERSION 3.12)
project(OpenOBLScripting)

add_library(OpenOBLScripting STATIC)
add_library(OpenOBL::OpenOBLScripting ALIAS OpenOBLScripting)

target_include_directories(OpenOBLScripting PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLScripting PRIVATE
        ${CMAKE_SOURCE_DIR}/include/scripting/console_engine.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/logging.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/script_engine.hpp
        ${CMAKE_SOURCE_DIR}/include/scripting/script_engine_base.hpp
        ast.cpp
        ast.hpp
        console_engine.cpp
        grammar.cpp
        grammar.hpp
        jit.cpp
        jit.hpp
        llvm.cpp
        llvm.hpp
        pegtl.hpp
        script_engine.cpp
        script_engine_base.cpp)

target_include_directories(OpenOBLScripting PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(OpenOBLScripting PUBLIC ${LLVM_DEFINITIONS})

if (OO_USE_SHARED_LLVM_LIBS)
    llvm_config(OpenOBLScripting USE_SHARED engine orcjit passes)
else ()
    llvm_config(OpenOBLScripting engine orcjit passes)
endif ()

target_link_libraries(OpenOBLScripting
        PRIVATE MicrosoftGSL::GSL taocpp::pegtl
        PUBLIC OpenOBL::OpenOBLUtil em::nostdx spdlog::spdlog)

install(TARGETS OpenOBLScripting EXPORT OpenOBLScriptingTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLScriptingTargets
        FILE OpenOBLScriptingTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/scripting/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/scripting)