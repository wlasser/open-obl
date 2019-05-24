target_include_directories(OpenOBLTest PRIVATE scripting)
target_sources(OpenOBLTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/grammar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/llvm.cpp)
