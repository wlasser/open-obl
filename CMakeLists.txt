cmake_minimum_required(VERSION 3.12)
project(OpenOblivion)

include(CheckIPOSupported)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OO_USE_SHARED_LLVM_LIBS "Link against LLVM shared libraries" ON)
option(OO_USE_LTO "Use link-time optimization (LTO)" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(Catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.7.0)
FetchContent_Declare(PEGTL
        GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
        GIT_TAG cb86441d257dc4bfe28f6f53a8e1974496797fc3)
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.3.1)

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
if (OO_USE_LTO)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT)
    if (IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${OO_USE_LTO})
    else ()
        message(WARNING "IPO/LTO is not supported: ${IPO_OUTPUT}")
    endif ()
endif ()

find_package(Boost 1.67 REQUIRED COMPONENTS fiber)
find_package(Bullet 2.85 REQUIRED)
find_package(LLVM 7.0.0 REQUIRED CONFIG)
find_package(OGRE 1.11 REQUIRED)
find_package(pugixml REQUIRED)
find_package(SDL2 2.0 REQUIRED)
find_package(ZLIB 1.2.11 REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")

add_subdirectory(lib/gsl)
add_subdirectory(lib/polymorphic_value)
add_subdirectory(lib/tl)
add_subdirectory(lib/soloud)

# Globally set warning flags for all internal dependencies.
# External dependencies (above) can keep their own warning flags.
add_compile_options(-Wall -Wextra)

add_subdirectory(src/bsa)
add_subdirectory(src/fs)
add_subdirectory(src/gui)
add_subdirectory(src/io)
add_subdirectory(src/math)
add_subdirectory(src/mesh)
add_subdirectory(src/nif)
add_subdirectory(src/nifloader)
add_subdirectory(src/record)
add_subdirectory(src/ogre)
add_subdirectory(src/ogrebullet)
add_subdirectory(src/ogreimgui)
add_subdirectory(src/ogresoloud)
add_subdirectory(src/scripting)
add_subdirectory(src/sdl)

add_subdirectory(bsatools)
add_subdirectory(test)

add_executable(OpenOblivion)

target_include_directories(OpenOblivion PUBLIC include)
target_sources(OpenOblivion PRIVATE
        include/application.hpp
        include/application_context.hpp
        include/atmosphere.hpp
        include/bitflag.hpp
        include/bsa_archive_factory.hpp
        include/bullet/collision.hpp
        include/bullet/configuration.hpp
        include/cell_cache.hpp
        include/character_controller/abilities.hpp
        include/character_controller/animation.hpp
        include/character_controller/body.hpp
        include/character_controller/character.hpp
        include/character_controller/character_controller.hpp
        include/character_controller/character_controller_impl.hpp
        include/character_controller/fallback_state.hpp
        include/character_controller/jump_state.hpp
        include/character_controller/movement.hpp
        include/character_controller/run_state.hpp
        include/character_controller/sneak_jump_state.hpp
        include/character_controller/sneak_stand_state.hpp
        include/character_controller/stand_state.hpp
        include/character_controller/walk_state.hpp
        include/console_functions.hpp
        include/controls.hpp
        include/deferred_light_pass.hpp
        include/enum_template.hpp
        include/esp.hpp
        include/esp_coordinator.hpp
        include/exterior_manager.hpp
        include/game_settings.hpp
        include/globals.hpp
        include/initial_record_visitor.hpp
        include/job/job.hpp
        include/meta.hpp
        include/modes/console_mode.hpp
        include/modes/debug_draw_impl.hpp
        include/modes/game_mode.hpp
        include/modes/load_menu_mode.hpp
        include/modes/loading_menu_mode.hpp
        include/modes/main_menu_mode.hpp
        include/modes/menu_mode.hpp
        include/modes/menu_mode_base.hpp
        include/modes/mode.hpp
        include/persistent_reference_locator.hpp
        include/resolvers/acti_resolver.hpp
        include/resolvers/cell_resolver.hpp
        include/resolvers/cont_resolver.hpp
        include/resolvers/door_resolver.hpp
        include/resolvers/flor_resolver.hpp
        include/resolvers/furn_resolver.hpp
        include/resolvers/helpers.hpp
        include/resolvers/light_resolver.hpp
        include/resolvers/misc_resolver.hpp
        include/resolvers/npc_resolver.hpp
        include/resolvers/resolvers.hpp
        include/resolvers/static_resolver.hpp
        include/resolvers/wrld_resolver.hpp
        include/save_state.hpp
        include/scene_manager.hpp
        include/script_functions.hpp
        include/settings.hpp
        include/terrain_material_generator.hpp
        include/time_manager.hpp
        include/wrld.hpp
        src/application.cpp
        src/application_context.cpp
        src/atmosphere.cpp
        src/bsa_archive_factory.cpp
        src/bullet/collision.cpp
        src/cell_cache.cpp
        src/character_controller/animation.cpp
        src/character_controller/body.cpp
        src/character_controller/character.cpp
        src/character_controller/character_controller.cpp
        src/character_controller/character_controller_impl.cpp
        src/character_controller/jump_state.cpp
        src/character_controller/movement.cpp
        src/character_controller/run_state.cpp
        src/character_controller/sneak_jump_state.cpp
        src/character_controller/sneak_stand_state.cpp
        src/character_controller/stand_state.cpp
        src/character_controller/walk_state.cpp
        src/console_functions.cpp
        src/deferred_light_pass.cpp
        src/esp_coordinator.cpp
        src/exterior_manager.cpp
        src/game_settings.cpp
        src/globals.cpp
        src/initial_record_visitor.cpp
        src/main.cpp
        src/modes/console_mode.cpp
        src/modes/debug_draw_impl.cpp
        src/modes/game_mode.cpp
        src/modes/load_menu_mode.cpp
        src/modes/loading_menu_mode.cpp
        src/modes/main_menu_mode.cpp
        src/modes/menu_mode_base.cpp
        src/persistent_reference_locator.cpp
        src/resolvers/acti_resolver.cpp
        src/resolvers/cell_resolver.cpp
        src/resolvers/cont_resolver.cpp
        src/resolvers/door_resolver.cpp
        src/resolvers/flor_resolver.cpp
        src/resolvers/furn_resolver.cpp
        src/resolvers/helpers.cpp
        src/resolvers/light_resolver.cpp
        src/resolvers/misc_resolver.cpp
        src/resolvers/npc_resolver.cpp
        src/resolvers/static_resolver.cpp
        src/resolvers/wrld_resolver.cpp
        src/save_state.cpp
        src/scene_manager.cpp
        src/script_functions.cpp
        src/terrain_material_generator.cpp
        src/time_manager.cpp)

target_include_directories(OpenOblivion PRIVATE
        ${BULLET_INCLUDE_DIRS})

target_link_libraries(OpenOblivion PRIVATE
        OpenOblivion::OpenOblivionBsa
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionGui
        OpenOblivion::OpenOblivionIO
        OpenOblivion::OpenOblivionMath
        OpenOblivion::OpenOblivionMesh
        OpenOblivion::OpenOblivionNif
        OpenOblivion::OpenOblivionNifloader
        OpenOblivion::OpenOblivionOgre
        OpenOblivion::OpenOblivionOgreBullet
        OpenOblivion::OpenOblivionOgreImGui
        OpenOblivion::OpenOblivionOgreSoLoud
        OpenOblivion::OpenOblivionRecord
        OpenOblivion::OpenOblivionScripting
        OpenOblivion::OpenOblivionSDL
        Boost::boost
        Boost::fiber
        MicrosoftGSL::GSL
        OgreMain
        OgreGLSupport
        OgreOverlay
        OgreTerrain
        jbcoe::polymorphic_value
        RenderSystem_GL3Plus
        SDL2::SDL2
        SDL2::SDL2main
        SoLoud::SoLoud
        spdlog::spdlog
        stdc++fs
        tloptional
        ZLIB::ZLIB
        ${BULLET_LIBRARIES})
