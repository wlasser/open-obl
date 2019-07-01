target_include_directories(OpenOBLTest PRIVATE scripting)
target_sources(OpenOBLTest PRIVATE
        ${CMAKE_SOURCE_DIR}/src/scripting/ast.hpp
        ${CMAKE_SOURCE_DIR}/src/scripting/grammar.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/grammar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/llvm.cpp)

if (MSVC)
    # TODO: Export symbols properly on MSVC
else ()
    target_link_options(OpenOBLTest PRIVATE -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/symbols.list)
    target_sources(OpenOBLTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/symbols.list)
endif ()
