cmake_minimum_required(VERSION 3.12)
project(OpenOblivion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

find_package(ZLIB 1.2.11 REQUIRED)
find_package(OGRE 1.11 REQUIRED COMPONENTS Bites RTShaderSystem)

file(COPY ${OGRE_CONFIG_DIR}/resources.cfg DESTINATION ${CMAKE_BINARY_DIR})

add_executable(OpenOblivion
        src/main.cpp
        src/bsa.cpp
        src/esp.cpp
        src/io_util.cpp
        src/records.cpp
        src/save_state.cpp
        src/system_time.cpp
        src/app.cpp)
target_include_directories(OpenOblivion PUBLIC include)

target_include_directories(OpenOblivion PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(OpenOblivion PRIVATE ${ZLIB_LIBRARIES} ${OGRE_LIBRARIES})
