target_include_directories(OpenOblivionTest PRIVATE scripting)
target_sources(OpenOblivionTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/grammar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/llvm.cpp)
