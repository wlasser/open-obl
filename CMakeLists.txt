cmake_minimum_required(VERSION 3.12)
project(OpenOblivion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fstandalone-debug")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/lib/modules")

find_package(ZLIB 1.2.11 REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OGRE 1.11 REQUIRED COMPONENTS)
find_package(Boost 1.67 REQUIRED)
find_package(Bullet 2.85 REQUIRED)

set(SPDLOG_BUILD_EXAMPLES NO)
set(SPDLOG_BUILD_TESTING NO)
set(SPDLOG_BUILD_BENCH NO)
add_subdirectory(lib/spdlog)

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
        src/nif/bhk.cpp
        src/nif/compound.cpp
        src/nif/niobject.cpp
        include/nif/basic.hpp
        include/nif/bhk.hpp
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
        include/record/record_header.hpp
        include/record/size_of.hpp
        include/record/subrecord.hpp
        include/record/tuplifiable.hpp)

# Ogre library
add_library(OpenOblivionOgre OBJECT)
target_include_directories(OpenOblivionOgre PUBLIC include)
target_sources(OpenOblivionOgre PUBLIC
        src/ogre/ogre_stream_wrappers.cpp
        src/ogre/window.cpp
        include/ogre/ogre_stream_wrappers.hpp
        include/ogre/spdlog_listener.hpp
        include/ogre/window.hpp)
target_link_libraries(OpenOblivionOgre PUBLIC
        spdlog
        ${OGRE_LIBRARIES})

# OgreBullet library
add_library(OpenOblivionOgreBullet OBJECT)
target_include_directories(OpenOblivionOgreBullet PUBLIC include)
target_sources(OpenOblivionOgreBullet PUBLIC
        $<TARGET_OBJECTS:OpenOblivionNif>
        $<TARGET_OBJECTS:OpenOblivionOgre>
        src/ogrebullet/collision_object.cpp
        src/ogrebullet/collision_object_manager.cpp
        src/ogrebullet/debug_drawer.cpp
        src/ogrebullet/motion_state.cpp
        src/ogrebullet/rigid_body.cpp
        include/ogrebullet/collision_object.hpp
        include/ogrebullet/collision_object_manager.hpp
        include/ogrebullet/debug_drawer.hpp
        include/ogrebullet/motion_state.hpp
        include/ogrebullet/rigid_body.hpp)
target_include_directories(OpenOblivionOgreBullet PUBLIC
        ${BULLET_INCLUDE_DIRS})
target_link_libraries(OpenOblivionOgreBullet PUBLIC
        spdlog
        ${OGRE_LIBRARIES}
        ${BULLET_LIBRARIES})

# Engine library
add_library(OpenOblivionEngine OBJECT)
target_include_directories(OpenOblivionEngine PUBLIC include)
target_sources(OpenOblivionEngine PUBLIC
        $<TARGET_OBJECTS:OpenOblivionIO>
        $<TARGET_OBJECTS:OpenOblivionNif>
        $<TARGET_OBJECTS:OpenOblivionRecord>
        $<TARGET_OBJECTS:OpenOblivionOgre>
        $<TARGET_OBJECTS:OpenOblivionOgreBullet>
        src/engine/application.cpp
        src/engine/bsa.cpp
        src/sdl/sdl.cpp
        src/engine/initial_processor.cpp
        src/engine/managers/interior_cell_manager.cpp
        src/engine/managers/light_manager.cpp
        src/engine/nifloader/loader.cpp
        src/engine/nifloader/loader_state.cpp
        src/engine/nif_collision_object_loader.cpp
        src/engine/nif_collision_object_loader_state.cpp
        src/engine/player_controller.cpp
        include/engine/application.hpp
        include/engine/bsa.hpp
        include/bullet/configuration.hpp
        include/engine/conversions.hpp
        include/sdl/sdl.hpp
        include/engine/initial_processor.hpp
        include/engine/keep_strategy.hpp
        include/engine/managers/interior_cell_manager.hpp
        include/engine/managers/light_manager.hpp
        include/engine/managers/static_manager.hpp
        include/engine/nifloader/loader.hpp
        include/engine/nifloader/loader_state.hpp
        include/engine/nif_collision_object_loader.hpp
        include/engine/nif_collision_object_loader_state.hpp
        include/engine/player_controller.hpp
        include/engine/settings.hpp)

target_include_directories(OpenOblivionEngine PUBLIC
        ${SDL2_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${BULLET_INCLUDE_DIRS})

target_link_libraries(OpenOblivionEngine PUBLIC
        ${OGRE_LIBRARIES}
        ${SDL2_LIBRARY}
        ${BULLET_LIBRARIES}
        spdlog
        stdc++fs)

add_executable(OpenOblivion)
target_include_directories(OpenOblivion PUBLIC include)
target_sources(OpenOblivion PRIVATE
        $<TARGET_OBJECTS:OpenOblivionIO>
        $<TARGET_OBJECTS:OpenOblivionNif>
        $<TARGET_OBJECTS:OpenOblivionRecord>
        $<TARGET_OBJECTS:OpenOblivionEngine>
        $<TARGET_OBJECTS:OpenOblivionOgre>
        $<TARGET_OBJECTS:OpenOblivionOgreBullet>
        src/bsa/bsa.cpp
        src/formid.cpp
        src/main.cpp
        src/records.cpp
        src/save_state.cpp
        src/system_time.cpp
        include/actor_value.hpp
        include/attribute.hpp
        include/bsa/bsa.hpp
        include/esp.hpp
        include/formid.hpp
        include/game_settings.hpp
        include/magic_effects.hpp
        include/save_state.hpp
        include/system_time.hpp)

target_include_directories(OpenOblivion PRIVATE
        ${ZLIB_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${BULLET_INCLUDE_DIRS})
target_link_libraries(OpenOblivion PRIVATE
        ${ZLIB_LIBRARIES}
        ${OGRE_LIBRARIES}
        ${SDL2_LIBRARY}
        ${BULLET_LIBRARIES}
        spdlog
        stdc++fs)

find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(GTKMM gtkmm-3.0)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources/bsabrowser)

    add_custom_command(OUTPUT resources/bsabrowser/resources.c
            COMMAND glib-compile-resources --target=${CMAKE_CURRENT_BINARY_DIR}/resources/bsabrowser/resources.c --generate-source bsabrowser.gresource.xml
            DEPENDS resources/bsabrowser/bsabrowser.gresource.xml resources/bsabrowser/window.glade
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/bsabrowser/)

    add_executable(BSABrowser)
    target_include_directories(BSABrowser PRIVATE include)
    target_sources(BSABrowser PRIVATE
            $<TARGET_OBJECTS:OpenOblivionIO>
            src/bsa/application.cpp
            src/bsa/bsa.cpp
            src/bsa/main.cpp
            include/bsa/application.hpp
            include/bsa/bsa.hpp
            resources/bsabrowser/resources.c)

    target_include_directories(BSABrowser PRIVATE
            ${GTKMM_INCLUDE_DIRS})
    target_link_libraries(BSABrowser PRIVATE
            ${ZLIB_LIBRARIES}
            ${GTKMM_LIBRARIES}
            spdlog
            stdc++fs)
endif ()

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE lib/catch)

add_executable(Test test/tests.cpp)
target_include_directories(Test PRIVATE include)
target_sources(Test PRIVATE
        $<TARGET_OBJECTS:OpenOblivionIO>
        test/io/write_bytes.cpp)
target_link_libraries(Test PRIVATE Catch)