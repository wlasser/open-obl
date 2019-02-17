cmake_minimum_required(VERSION 3.12)
project(OpenOblivion)

include(CheckIPOSupported)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

option(USE_SHARED_LLVM_LIBS "Link against LLVM shared libraries" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_QUIET OFF)

# As of commit a06c4a1 Abseil has a bug where GoogleTest cannot be downloaded
# when Abseil is included as a subdirectory. Turn this option on and remove the
# FetchContent for GoogleTest when this is fixed.
# TODO: Let Abseil download GoogleTest
set(ABSL_USE_GOOGLETEST_HEAD OFF CACHE BOOL "")
set(ABSL_RUN_TESTS ON CACHE BOOL "")

FetchContent_Declare(GoogleTest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG origin/master)
FetchContent_Declare(Abseil
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG origin/master)
FetchContent_Declare(Catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.5.0)
FetchContent_Declare(PEGTL
        GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
        GIT_TAG cb86441d257dc4bfe28f6f53a8e1974496797fc3)
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.3.0)

FetchContent_GetProperties(GoogleTest)
if (NOT GoogleTest_populated)
    FetchContent_Populate(GoogleTest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif ()

FetchContent_GetProperties(Abseil)
if (NOT Abseil_populated)
    FetchContent_Populate(Abseil)
    add_subdirectory(${abseil_SOURCE_DIR} ${abseil_BINARY_DIR})
endif ()

FetchContent_GetProperties(Catch)
if (NOT Catch_populated)
    FetchContent_Populate(Catch)
    add_subdirectory(${catch_SOURCE_DIR} ${catch_BINARY_DIR})
endif ()

FetchContent_GetProperties(PEGTL)
if (NOT PEGTL_populated)
    FetchContent_Populate(PEGTL)
    add_subdirectory(${pegtl_SOURCE_DIR} ${pegtl_BINARY_DIR})
endif ()

FetchContent_GetProperties(spdlog)
if (NOT spdlog_populated)
    FetchContent_Populate(spdlog)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif ()

# Link-time optimization
cmake_policy(SET CMP0069 NEW)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT)
if (IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
else ()
    message(WARNING "IPO/LTO is not supported: ${IPO_OUTPUT}")
endif ()

find_package(ZLIB 1.2.11 REQUIRED)
find_package(SDL2 2.0 REQUIRED)
find_package(OGRE 1.11 REQUIRED)
find_package(Boost 1.67 REQUIRED)
find_package(Bullet 2.85 REQUIRED)
find_package(pugixml REQUIRED)
find_package(LLVM 7.0.0 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")

add_subdirectory(lib/gsl)
add_subdirectory(lib/polymorphic_value)
add_subdirectory(lib/tl)
add_subdirectory(lib/soloud)

add_subdirectory(src/bsa)
add_subdirectory(src/fs)
add_subdirectory(src/gui)
add_subdirectory(src/io)
add_subdirectory(src/nif)
add_subdirectory(src/record)
add_subdirectory(src/ogre)
add_subdirectory(src/ogrebullet)
add_subdirectory(src/ogreimgui)
add_subdirectory(src/ogresoloud)
add_subdirectory(src/sdl)
add_subdirectory(src/scripting)

add_subdirectory(test)
add_subdirectory(bsatools)

add_executable(OpenOblivion)
target_compile_options(OpenOblivion PRIVATE -Wall -Wextra)
target_compile_features(OpenOblivion PUBLIC cxx_std_17)
set_property(TARGET OpenOblivion PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOblivion PUBLIC include)
target_sources(OpenOblivion PRIVATE
        src/application.cpp
        src/application_context.cpp
        src/bsa_archive_factory.cpp
        src/character_controller/jump_state.cpp
        src/character_controller/player_controller.cpp
        src/character_controller/player_controller_impl.cpp
        src/character_controller/run_state.cpp
        src/character_controller/sneak_jump_state.cpp
        src/character_controller/sneak_stand_state.cpp
        src/character_controller/stand_state.cpp
        src/character_controller/walk_state.cpp
        src/console_functions.cpp
        src/esp_coordinator.cpp
        src/fnt_loader.cpp
        src/game_settings.cpp
        src/initial_record_visitor.cpp
        src/main.cpp
        src/modes/console_mode.cpp
        src/modes/game_mode.cpp
        src/modes/load_menu_mode.cpp
        src/modes/loading_menu_mode.cpp
        src/modes/main_menu_mode.cpp
        src/modes/menu_mode_base.cpp
        src/nifloader/animation.cpp
        src/nifloader/loader.cpp
        src/nifloader/loader_state.cpp
        src/nifloader/collision_object_loader.cpp
        src/nifloader/collision_object_loader_state.cpp
        src/nifloader/mesh_loader.cpp
        src/nifloader/mesh_loader_state.cpp
        src/nifloader/nif_resource.cpp
        src/nifloader/nif_resource_manager.cpp
        src/nifloader/scene.cpp
        src/nifloader/skeleton_loader.cpp
        src/nifloader/skeleton_loader_state.cpp
        src/resolvers/acti_resolver.cpp
        src/resolvers/cell_resolver.cpp
        src/resolvers/door_resolver.cpp
        src/resolvers/light_resolver.cpp
        src/resolvers/npc_resolver.cpp
        src/resolvers/helpers.cpp
        src/resolvers/static_resolver.cpp
        src/resolvers/wrld_resolver.cpp
        src/save_state.cpp
        src/terrain_material_generator.cpp
        include/application.hpp
        include/application_context.hpp
        include/bitflag.hpp
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
        include/console_functions.hpp
        include/controls.hpp
        include/conversions.hpp
        include/enum_template.hpp
        include/esp.hpp
        include/esp_coordinator.hpp
        include/fnt_loader.hpp
        include/game_settings.hpp
        include/initial_record_visitor.hpp
        include/keep_strategy.hpp
        include/meta.hpp
        include/modes/console_mode.hpp
        include/modes/game_mode.hpp
        include/modes/load_menu_mode.hpp
        include/modes/loading_menu_mode.hpp
        include/modes/main_menu_mode.hpp
        include/modes/menu_mode.hpp
        include/modes/menu_mode_base.hpp
        include/modes/mode.hpp
        include/nifloader/animation.hpp
        include/nifloader/loader.hpp
        include/nifloader/loader_state.hpp
        include/nifloader/collision_object_loader.hpp
        include/nifloader/collision_object_loader_state.hpp
        include/nifloader/mesh_loader.hpp
        include/nifloader/mesh_loader_state.hpp
        include/nifloader/nif_resource.hpp
        include/nifloader/nif_resource_manager.hpp
        include/nifloader/scene.hpp
        include/nifloader/skeleton_loader.hpp
        include/nifloader/skeleton_loader_state.hpp
        include/resolvers/acti_resolver.hpp
        include/resolvers/cell_resolver.hpp
        include/resolvers/door_resolver.hpp
        include/resolvers/ecs.hpp
        include/resolvers/helpers.hpp
        include/resolvers/light_resolver.hpp
        include/resolvers/npc_resolver.hpp
        include/resolvers/resolvers.hpp
        include/resolvers/static_resolver.hpp
        include/resolvers/wrld_resolver.hpp
        include/save_state.hpp
        include/settings.hpp
        include/terrain_material_generator.hpp)

target_include_directories(OpenOblivion PRIVATE
        ${BULLET_INCLUDE_DIRS})

target_link_libraries(OpenOblivion PRIVATE
        OpenOblivion::OpenOblivionBsa
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionGui
        OpenOblivion::OpenOblivionIO
        OpenOblivion::OpenOblivionNif
        OpenOblivion::OpenOblivionRecord
        OpenOblivion::OpenOblivionOgre
        OpenOblivion::OpenOblivionOgreBullet
        OpenOblivion::OpenOblivionOgreImGui
        OpenOblivion::OpenOblivionOgreSoLoud
        OpenOblivion::OpenOblivionSDL
        OpenOblivion::OpenOblivionScripting
        Boost::boost
        ZLIB::ZLIB
        SDL2::SDL2
        SDL2::SDL2main
        SoLoud::SoLoud
        OgreMain OgreGLSupport OgreOverlay OgreTerrain
        spdlog::spdlog
        stdc++fs
        tloptional
        GSL
        polymorphic_value
        absl::flat_hash_map
        ${BULLET_LIBRARIES})
