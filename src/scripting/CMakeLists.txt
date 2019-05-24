cmake_minimum_required(VERSION 3.12)
project(OpenOBLScripting)

add_library(OpenOBLScripting STATIC)
add_library(OpenOBL::OpenOBLScripting ALIAS OpenOBLScripting)

target_include_directories(OpenOBLScripting PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLScripting PRIVATE
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

target_include_directories(OpenOBLScripting PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(OpenOBLScripting PRIVATE ${LLVM_DEFINITIONS})

if (OO_USE_SHARED_LLVM_LIBS)
    llvm_config(OpenOBLScripting USE_SHARED engine)
else ()
    llvm_config(OpenOBLScripting engine)
endif ()

target_link_libraries(OpenOBLScripting
        PRIVATE taocpp::pegtl MicrosoftGSL::GSL
        PUBLIC OpenOBL::OpenOBLUtil ${llvm_libs} spdlog::spdlog)

set_target_properties(OpenOBLScripting PROPERTIES
        CXX_VISIBILITY_PRESET hidden)
target_link_libraries(OpenOBLScripting PRIVATE "-Wl,--export-dynamic")

install(TARGETS OpenOBLScripting EXPORT OpenOBLScriptingTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLScriptingTargets
        FILE OpenOBLScriptingTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/scripting/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/scripting)