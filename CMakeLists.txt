cmake_minimum_required(VERSION 3.12)
project(OpenOblivion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

find_package(ZLIB 1.2.11 REQUIRED)
find_package(OGRE 1.11 REQUIRED COMPONENTS Bites RTShaderSystem)

file(COPY ${OGRE_CONFIG_DIR}/resources.cfg DESTINATION ${CMAKE_BINARY_DIR})

add_executable(OpenOblivion)
target_sources(OpenOblivion PRIVATE
        src/app.cpp
        src/bsa.cpp
        src/esp.cpp
        src/main.cpp
        src/io/string.cpp
        src/io/read_bytes.cpp
        src/nif/basic.cpp
        src/nif/compound.cpp
        src/nif/nif.cpp
        src/records.cpp
        src/record/group.cpp
        src/record/io.cpp
        src/save_state.cpp
        src/system_time.cpp)
target_sources(OpenOblivion PRIVATE
        include/actor_value.hpp
        include/app.hpp
        include/attribute.hpp
        include/bsa.hpp
        include/esp.hpp
        include/formid.hpp
        include/io/memstream.hpp
        include/io/io.hpp
        include/io/string.hpp
        include/io/read_bytes.hpp
        include/io/write_bytes.hpp
        include/magic_effects.hpp
        include/nif/basic.hpp
        include/nif/compound.hpp
        include/nif/enum.hpp
        include/nif/nif.hpp
        include/nif/versionable.hpp
        include/records.hpp
        include/record/exceptions.hpp
        include/record/group.hpp
        include/record/io.hpp
        include/record/rec_of.hpp
        include/record/size_of.hpp
        include/record/subrecord.hpp
        include/record/tuplifiable.hpp
        include/save_state.hpp
        include/system_time.hpp)
target_include_directories(OpenOblivion PUBLIC include)

target_include_directories(OpenOblivion PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(OpenOblivion PRIVATE ${ZLIB_LIBRARIES} ${OGRE_LIBRARIES})
