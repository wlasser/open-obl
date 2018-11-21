target_include_directories(OpenOblivionTest PRIVATE scripting)
target_sources(OpenOblivionTest PRIVATE
        scripting/helpers.hpp
        scripting/grammar.cpp
        scripting/helpers.cpp
        scripting/llvm.cpp)
