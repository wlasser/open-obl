cmake_minimum_required(VERSION 3.13)
project(OpenOBL)

include(CheckIPOSupported)
include(FetchContent)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OO_USE_SHARED_LLVM_LIBS "Link against LLVM shared libraries" ON)
option(OO_USE_LTO "Use link-time optimization (LTO)" OFF)
option(OO_USE_EXTERNAL_BULLET "Find Bullet using find_package instead of FetchContent" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(Bullet
        GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
        GIT_TAG 2.88)
FetchContent_Declare(Catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.7.0)
FetchContent_Declare(PEGTL
        GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
        GIT_TAG cb86441d257dc4bfe28f6f53a8e1974496797fc3)
FetchContent_Declare(pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG v1.9)
# Fixed to latest commit as of writing to get rid of -Wshadow warnings under
# g++.
# TODO: Update to v1.3.2 when it is released.
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG 74dbf4cf702b49c98642c9afe74d114a238a6a07)

if (OO_USE_EXTERNAL_BULLET)
    find_package(Bullet 2.88 REQUIRED MODULE)
else ()
    FetchContent_GetProperties(Bullet)
    if (NOT Bullet_populated)
        FetchContent_Populate(Bullet)
        set(USE_GRAPHICAL_BENCHMARK OFF CACHE STRING "Use Bullet Graphical Benchmark")
        set(BUILD_CPU_DEMOS OFF CACHE STRING "Build Bullet CPU examples")
        set(USE_GLUT OFF CACHE STRING "Use Glut")
        set(BUILD_BULLET3 OFF CACHE STRING "Whether to build Bullet 3")
        set(BUILD_PYBULLET OFF CACHE STRING "Whether to build pybullet")
        set(BUILD_ENET OFF CACHE STRING "Build Bullet with enet UDP networking support")
        set(BUILD_CLSOCKET OFF CACHE STRING "Build Bullet with enet TCP networking support")
        set(BUILD_EGL OFF CACHE STRING "Build OpenGL/EGL for Bullet")
        set(BUILD_BULLET2_DEMOS OFF CACHE STRING "Build Bullet 2 demos")
        set(BUILD_EXTRAS OFF CACHE STRING "Build Bullet extras")
        set(INSTALL_LIBS ON CACHE STRING "Whether to install the Bullet libraries")
        set(BUILD_UNIT_TESTS OFF CACHE STRING "Build Bullet unit tests")
        add_subdirectory(${bullet_SOURCE_DIR} ${bullet_BINARY_DIR})

        function(oo_add_bullet_target TargetName)
            add_library("Bullet::${TargetName}" ALIAS ${TargetName})
            target_include_directories(${TargetName} PUBLIC
                    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/bullet-src/src>
                    $<INSTALL_INTERFACE:include>)
            install(TARGETS ${TargetName}
                    EXPORT Bullet_${TargetName}Targets
                    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
            install(EXPORT Bullet_${TargetName}Targets
                    FILE Bullet_${TargetName}Targets.cmake
                    NAMESPACE Bullet::
                    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)
        endfunction()

        oo_add_bullet_target(LinearMath)

        oo_add_bullet_target(BulletCollision)
        target_link_libraries(BulletCollision PUBLIC LinearMath)

        oo_add_bullet_target(BulletDynamics)
        target_link_libraries(BulletDynamics PUBLIC LinearMath BulletCollision)
    endif ()
endif ()

FetchContent_GetProperties(Catch)
if (NOT Catch_populated)
    FetchContent_Populate(Catch)
    add_subdirectory(${catch_SOURCE_DIR} ${catch_BINARY_DIR})
endif ()

FetchContent_GetProperties(PEGTL)
if (NOT PEGTL_populated)
    FetchContent_Populate(PEGTL)
    set(PEGTL_BUILD_EXAMPLES OFF CACHE STRING "Build PEGTL example programs")
    add_subdirectory(${pegtl_SOURCE_DIR} ${pegtl_BINARY_DIR})
endif ()

FetchContent_GetProperties(pugixml)
if (NOT pugixml_populated)
    FetchContent_Populate(pugixml)
    add_subdirectory(${pugixml_SOURCE_DIR} ${pugixml_BINARY_DIR})
endif ()

FetchContent_GetProperties(spdlog)
if (NOT spdlog_populated)
    FetchContent_Populate(spdlog)
    set(SPDLOG_INSTALL ON CACHE STRING "Generate the spdlog install target")
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
find_package(LLVM 7.1.0 REQUIRED CONFIG)
find_package(OGRE 1.11 REQUIRED COMPONENTS Overlay Paging Terrain)
find_package(SDL2 2.0 REQUIRED)
find_package(ZLIB 1.2.11 REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")

set(GSL_CXX_STANDARD 17 CACHE STRING "C++ standard used by GSL")
add_subdirectory(lib/gsl)
add_subdirectory(lib/polymorphic_value)
add_subdirectory(lib/tl)
add_subdirectory(lib/soloud)

# Globally set warning flags for all internal dependencies.
# External dependencies (above) can keep their own warning flags.
add_compile_options(-Wall -Wextra -Wshadow -Wpedantic)

add_subdirectory(src/bsa)
add_subdirectory(src/config)
add_subdirectory(src/esp)
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
add_subdirectory(src/util)

add_subdirectory(bsatools)
add_subdirectory(test)

add_executable(OpenOBL)

target_include_directories(OpenOBL PUBLIC include)
target_sources(OpenOBL PRIVATE
        include/application.hpp
        include/application_context.hpp
        include/atmosphere.hpp
        include/audio.hpp
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
        include/chrono.hpp
        include/console_functions.hpp
        include/controls.hpp
        include/exterior_manager.hpp
        include/initial_record_visitor.hpp
        include/job/job.hpp
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
        include/resolvers/ligh_resolver.hpp
        include/resolvers/misc_resolver.hpp
        include/resolvers/npc__resolver.hpp
        include/resolvers/resolvers.hpp
        include/resolvers/stat_resolver.hpp
        include/resolvers/wrld_resolver.hpp
        include/save_state.hpp
        include/script_functions.hpp
        include/wrld.hpp
        src/application.cpp
        src/application_context.cpp
        src/atmosphere.cpp
        src/audio.cpp
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
        src/chrono.cpp
        src/console_functions.cpp
        src/exterior_manager.cpp
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
        src/resolvers/ligh_resolver.cpp
        src/resolvers/misc_resolver.cpp
        src/resolvers/npc__resolver.cpp
        src/resolvers/stat_resolver.cpp
        src/resolvers/wrld_resolver.cpp
        src/save_state.cpp
        src/script_functions.cpp
        src/wrld.cpp)

target_link_libraries(OpenOBL PRIVATE
        Bullet::BulletCollision
        Bullet::BulletDynamics
        OpenOBL::OpenOBLBsa
        OpenOBL::OpenOBLConfig
        OpenOBL::OpenOBLEsp
        OpenOBL::OpenOBLFS
        OpenOBL::OpenOBLGui
        OpenOBL::OpenOBLIO
        OpenOBL::OpenOBLMath
        OpenOBL::OpenOBLMesh
        OpenOBL::OpenOBLNif
        OpenOBL::OpenOBLNifloader
        OpenOBL::OpenOBLOgre
        OpenOBL::OpenOBLOgreBullet
        OpenOBL::OpenOBLOgreImGui
        OpenOBL::OpenOBLOgreSoLoud
        OpenOBL::OpenOBLRecord
        OpenOBL::OpenOBLScripting
        OpenOBL::OpenOBLSDL
        OpenOBL::OpenOBLUtil
        Boost::boost
        Boost::fiber
        MicrosoftGSL::GSL
        OgreMain
        OgreGLSupport
        OgreOverlay
        OgreTerrain
        RenderSystem_GL3Plus
        SDL2::SDL2
        SDL2::SDL2main
        spdlog::spdlog
        stdc++fs
        tl::tloptional)
