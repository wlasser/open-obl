cmake_minimum_required(VERSION 3.12)
project(OpenOblivion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

option(USE_IWYU "Run include-what-you-use when building" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_subdirectory(lib/polymorphic_value)

add_subdirectory(src/io)
add_subdirectory(src/fs)
add_subdirectory(src/nif)
add_subdirectory(src/record)
add_subdirectory(src/ogre)
add_subdirectory(src/ogrebullet)
add_subdirectory(src/ogreimgui)
add_subdirectory(src/sdl)

add_executable(OpenOblivion)
target_include_directories(OpenOblivion PUBLIC include)
target_sources(OpenOblivion PRIVATE
        src/application.cpp
        src/bsa/bsa.cpp
        src/bsa_archive_factory.cpp
        src/character_controller/jump_state.cpp
        src/character_controller/player_controller.cpp
        src/character_controller/run_state.cpp
        src/character_controller/sneak_jump_state.cpp
        src/character_controller/sneak_stand_state.cpp
        src/character_controller/stand_state.cpp
        src/character_controller/walk_state.cpp
        src/esp_coordinator.cpp
        src/formid.cpp
        src/gui/gui.cpp
        src/gui/stack/program.cpp
        src/gui/stack/types.cpp
        src/gui/trait.cpp
        src/gui/traits.cpp
        src/gui/trait_selector.cpp
        src/gui/xml.cpp
        src/initial_processor.cpp
        src/main.cpp
        src/nifloader/loader.cpp
        src/nifloader/loader_state.cpp
        src/nifloader/collision_object_loader.cpp
        src/nifloader/collision_object_loader_state.cpp
        src/nifloader/mesh_loader.cpp
        src/nifloader/mesh_loader_state.cpp
        src/records.cpp
        src/resolvers/door_resolver.cpp
        src/resolvers/interior_cell_resolver.cpp
        src/resolvers/light_resolver.cpp
        src/resolvers/resolvers.cpp
        src/resolvers/static_resolver.cpp
        src/save_state.cpp
        src/subrecords.cpp
        src/system_time.cpp
        include/actor_value.hpp
        include/application.hpp
        include/attribute.hpp
        include/bitflag.hpp
        include/bsa/bsa.hpp
        include/bsa_archive_factory.hpp
        include/bullet/collision.hpp
        include/bullet/configuration.hpp
        include/character_controller/abilities.hpp
        include/character_controller/fallback_state.hpp
        include/character_controller/jump_state.hpp
        include/character_controller/player_controller.hpp
        include/character_controller/player_controller_impl.hpp
        include/character_controller/run_state.hpp
        include/character_controller/sneak_jump_state.hpp
        include/character_controller/sneak_stand_state.hpp
        include/character_controller/stand_state.hpp
        include/character_controller/walk_state.hpp
        include/controls.hpp
        include/conversions.hpp
        include/enum_template.hpp
        include/esp.hpp
        include/esp_coordinator.hpp
        include/formid.hpp
        include/game_settings.hpp
        include/gui/elements/image.hpp
        include/gui/elements/rect.hpp
        include/gui/elements/text.hpp
        include/gui/gui.hpp
        include/gui/menu.hpp
        include/gui/menus/loading_menu.hpp
        include/gui/screen.hpp
        include/gui/stack/instructions.hpp
        include/gui/stack/meta.hpp
        include/gui/stack/program.hpp
        include/gui/stack/types.hpp
        include/gui/strings.hpp
        include/gui/trait.hpp
        include/gui/traits.hpp
        include/gui/trait_selector.hpp
        include/gui/ui_element.hpp
        include/gui/xml.hpp
        include/initial_processor.hpp
        include/keep_strategy.hpp
        include/magic_effects.hpp
        include/nifloader/loader.hpp
        include/nifloader/loader_state.hpp
        include/nifloader/collision_object_loader.hpp
        include/nifloader/collision_object_loader_state.hpp
        include/nifloader/mesh_loader.hpp
        include/nifloader/mesh_loader_state.hpp
        include/resolvers/door_resolver.hpp
        include/resolvers/helpers.hpp
        include/resolvers/interior_cell_resolver.hpp
        include/resolvers/light_resolver.hpp
        include/resolvers/resolvers.hpp
        include/resolvers/static_resolver.hpp
        include/save_state.hpp
        include/settings.hpp
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
        OpenOblivion::OpenOblivionOgreImGui
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
        polymorphic_value
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
        test/io/write_bytes.cpp
        test/engine/gui/stack.cpp)
target_sources(Test PRIVATE
        src/gui/stack/program.cpp
        src/gui/stack/types.cpp
        src/gui/trait.cpp
        src/gui/trait_selector.cpp
        src/gui/xml.cpp)
target_link_libraries(Test PRIVATE OpenOblivion::OpenOblivionIO Catch
        pugixml)
