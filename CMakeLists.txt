cmake_minimum_required(VERSION 3.12)
project(OpenOblivion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

find_package(ZLIB 1.2.11 REQUIRED)
find_package(OGRE 1.11 REQUIRED COMPONENTS Bites RTShaderSystem)
find_package(Boost 1.67 REQUIRED)

# IO library
add_library(OpenOblivionIO OBJECT)
target_include_directories(OpenOblivionIO PUBLIC include)
target_sources(OpenOblivionIO PUBLIC
        src/io/io.cpp
        src/io/read_bytes.cpp
        src/io/string.cpp
        include/io/io.hpp
        include/io/memstream.hpp
        include/io/read_bytes.hpp
        include/io/string.hpp
        include/io/write_bytes.hpp)

# Nif library
add_library(OpenOblivionNif OBJECT)
target_include_directories(OpenOblivionNif PUBLIC include)
target_sources(OpenOblivionNif PUBLIC
        $<TARGET_OBJECTS:OpenOblivionIO>
        src/nif/basic.cpp
        src/nif/compound.cpp
        src/nif/niobject.cpp
        include/nif/basic.hpp
        include/nif/compound.hpp
        include/nif/enum.hpp
        include/nif/niobject.hpp
        include/nif/versionable.hpp)

# Record library
add_library(OpenOblivionRecord OBJECT)
target_include_directories(OpenOblivionRecord PUBLIC include)
target_sources(OpenOblivionRecord PUBLIC
        $<TARGET_OBJECTS:OpenOblivionIO>
        src/record/group.cpp
        src/record/io.cpp
        include/record/exceptions.hpp
        include/record/group.hpp
        include/record/io.hpp
        include/record/rec_of.hpp
        include/record/record.hpp
        include/record/size_of.hpp
        include/record/subrecord.hpp
        include/record/tuplifiable.hpp)

add_executable(OpenOblivion)
target_include_directories(OpenOblivion PUBLIC include)
target_sources(OpenOblivion PRIVATE
        $<TARGET_OBJECTS:OpenOblivionIO>
        $<TARGET_OBJECTS:OpenOblivionNif>
        $<TARGET_OBJECTS:OpenOblivionRecord>
        src/app.cpp
        src/bsa.cpp
        src/engine/bsa.cpp
        src/engine/nif_loader.cpp
        src/esp.cpp
        src/main.cpp
        src/records.cpp
        src/save_state.cpp
        src/system_time.cpp
        include/actor_value.hpp
        include/app.hpp
        include/attribute.hpp
        include/bsa.hpp
        include/engine/bsa.hpp
        include/engine/nif_loader.hpp
        include/esp.hpp
        include/formid.hpp
        include/magic_effects.hpp
        include/save_state.hpp
        include/system_time.hpp)

target_include_directories(OpenOblivion PRIVATE
        ${ZLIB_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS})
target_link_libraries(OpenOblivion PRIVATE
        ${ZLIB_LIBRARIES}
        ${OGRE_LIBRARIES}
        stdc++fs)

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE lib/catch)

add_executable(Test test/tests.cpp)
target_include_directories(Test PRIVATE include)
target_sources(Test PRIVATE
        $<TARGET_OBJECTS:OpenOblivionIO>
        test/io/write_bytes.cpp)
target_link_libraries(Test PRIVATE Catch)