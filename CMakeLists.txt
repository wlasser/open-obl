cmake_minimum_required(VERSION 3.12)
project(OpenOblivion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fstandalone-debug -Wall")

option(USE_IWYU "Run include-what-you-use when building" OFF)

find_package(ZLIB 1.2.11 REQUIRED)
find_package(SDL2 2.0 REQUIRED)
find_package(OGRE 1.11 REQUIRED)
find_package(Boost 1.67 REQUIRED)
find_package(Bullet 2.85 REQUIRED)
find_package(pugixml REQUIRED)

set(SPDLOG_BUILD_EXAMPLES NO)
set(SPDLOG_BUILD_TESTING NO)
set(SPDLOG_BUILD_BENCH NO)
add_subdirectory(lib/spdlog)

add_subdirectory(lib/gsl)

add_subdirectory(src/io)
add_subdirectory(src/fs)
add_subdirectory(src/nif)
add_subdirectory(src/record)
add_subdirectory(src/ogre)
add_subdirectory(src/ogrebullet)
add_subdirectory(src/sdl)

# Engine library
add_library(OpenOblivionEngine OBJECT)
target_include_directories(OpenOblivionEngine PUBLIC include)
target_sources(OpenOblivionEngine PUBLIC
        src/engine/application.cpp
        src/engine/bsa.cpp
        src/engine/gui/gui.cpp
        src/engine/gui/trait.cpp
        src/engine/gui/traits.cpp
        src/engine/gui/trait_selector.cpp
        src/engine/gui/xml.cpp
        src/engine/initial_processor.cpp
        src/engine/nifloader/loader.cpp
        src/engine/nifloader/loader_state.cpp
        src/engine/nifloader/collision_object_loader.cpp
        src/engine/nifloader/collision_object_loader_state.cpp
        src/engine/nifloader/mesh_loader.cpp
        src/engine/nifloader/mesh_loader_state.cpp
        src/engine/player_controller.cpp
        src/engine/resolvers/door_resolver.cpp
        src/engine/resolvers/interior_cell_resolver.cpp
        src/engine/resolvers/light_resolver.cpp
        src/engine/resolvers/resolvers.cpp
        src/engine/resolvers/static_resolver.cpp
        include/bullet/collision.hpp
        include/bullet/configuration.hpp
        include/engine/application.hpp
        include/engine/bsa.hpp
        include/engine/conversions.hpp
        include/engine/gui/gui.hpp
        include/engine/gui/menu.hpp
        include/engine/gui/menus/loading_menu.hpp
        include/engine/gui/screen.hpp
        include/engine/gui/strings.hpp
        include/engine/gui/trait.hpp
        include/engine/gui/traits.hpp
        include/engine/gui/trait_selector.hpp
        include/engine/gui/ui_element.hpp
        include/engine/gui/xml.hpp
        include/engine/initial_processor.hpp
        include/engine/keep_strategy.hpp
        include/engine/nifloader/loader.hpp
        include/engine/nifloader/loader_state.hpp
        include/engine/nifloader/collision_object_loader.hpp
        include/engine/nifloader/collision_object_loader_state.hpp
        include/engine/nifloader/mesh_loader.hpp
        include/engine/nifloader/mesh_loader_state.hpp
        include/engine/player_controller.hpp
        include/engine/resolvers/door_resolver.hpp
        include/engine/resolvers/interior_cell_resolver.hpp
        include/engine/resolvers/light_resolver.hpp
        include/engine/resolvers/resolvers.hpp
        include/engine/resolvers/static_resolver.hpp
        include/engine/settings.hpp)

target_include_directories(OpenOblivionEngine PUBLIC
        ${BULLET_INCLUDE_DIRS})

target_link_libraries(OpenOblivionEngine PUBLIC
        OpenOblivion::OpenOblivionIO
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionNif
        OpenOblivion::OpenOblivionRecord
        OpenOblivion::OpenOblivionOgre
        OpenOblivion::OpenOblivionOgreBullet
        OpenOblivion::OpenOblivionSDL
        Boost::boost
        OgreMain
        spdlog
        stdc++fs
        GSL
        pugixml
        ${BULLET_LIBRARIES})

add_executable(OpenOblivion)
target_include_directories(OpenOblivion PUBLIC include)
target_sources(OpenOblivion PRIVATE
        $<TARGET_OBJECTS:OpenOblivionEngine>
        src/bsa/bsa.cpp
        src/formid.cpp
        src/main.cpp
        src/records.cpp
        src/save_state.cpp
        src/subrecords.cpp
        src/system_time.cpp
        include/actor_value.hpp
        include/attribute.hpp
        include/bitflag.hpp
        include/bsa/bsa.hpp
        include/enum_template.hpp
        include/esp.hpp
        include/formid.hpp
        include/game_settings.hpp
        include/magic_effects.hpp
        include/save_state.hpp
        include/subrecords.hpp
        include/system_time.hpp)

target_include_directories(OpenOblivion PRIVATE
        ${BULLET_INCLUDE_DIRS})

target_link_libraries(OpenOblivion PRIVATE
        OpenOblivion::OpenOblivionIO
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionNif
        OpenOblivion::OpenOblivionRecord
        OpenOblivion::OpenOblivionOgre
        OpenOblivion::OpenOblivionOgreBullet
        OpenOblivion::OpenOblivionSDL
        Boost::boost
        ZLIB::ZLIB
        SDL2::SDL2
        SDL2::SDL2main
        OgreMain OgreGLSupport
        spdlog
        stdc++fs
        GSL
        pugixml
        ${BULLET_LIBRARIES})

if (USE_IWYU)
    find_program(IWYU_PATH NAMES include-what-you-use iwyu)
    if (IWYU_PATH)
        set_property(TARGET OpenOblivionIO PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
        set_property(TARGET OpenOblivionNif PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
        set_property(TARGET OpenOblivionRecord PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
        set_property(TARGET OpenOblivionOgre PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
        set_property(TARGET OpenOblivionOgreBullet PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
        set_property(TARGET OpenOblivionSDL PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
        set_property(TARGET OpenOblivionEngine PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
        set_property(TARGET OpenOblivion PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
    endif ()
endif ()

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
            src/bsa/application.cpp
            src/bsa/bsa.cpp
            src/bsa/main.cpp
            include/bsa/application.hpp
            include/bsa/bsa.hpp
            resources/bsabrowser/resources.c)

    target_include_directories(BSABrowser PRIVATE
            ${GTKMM_INCLUDE_DIRS})
    target_link_libraries(BSABrowser PRIVATE
            OpenOblivion::OpenOblivionIO
            ${ZLIB_LIBRARIES}
            ${GTKMM_LIBRARIES}
            spdlog
            GSL
            stdc++fs)
endif ()

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE lib/catch)

add_executable(Test test/tests.cpp)
target_include_directories(Test PRIVATE include)
target_sources(Test PRIVATE
        test/io/write_bytes.cpp)
target_link_libraries(Test PRIVATE OpenOblivion::OpenOblivionIO Catch)
