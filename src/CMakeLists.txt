cmake_minimum_required(VERSION 3.12)
project(OpenOBLGame)

add_executable(OpenOBL)

target_include_directories(OpenOBL PUBLIC include)
target_sources(OpenOBL PRIVATE
        ${CMAKE_SOURCE_DIR}/include/application.hpp
        ${CMAKE_SOURCE_DIR}/include/application_context.hpp
        ${CMAKE_SOURCE_DIR}/include/atmosphere.hpp
        ${CMAKE_SOURCE_DIR}/include/audio.hpp
        ${CMAKE_SOURCE_DIR}/include/cell_cache.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/abilities.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/animation.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/body.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/character.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/character_controller.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/character_controller_impl.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/fallback_state.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/jump_state.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/movement.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/run_state.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/sneak_jump_state.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/sneak_stand_state.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/stand_state.hpp
        ${CMAKE_SOURCE_DIR}/include/character_controller/walk_state.hpp
        ${CMAKE_SOURCE_DIR}/include/chrono.hpp
        ${CMAKE_SOURCE_DIR}/include/console_functions.hpp
        ${CMAKE_SOURCE_DIR}/include/controls.hpp
        ${CMAKE_SOURCE_DIR}/include/exterior_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/initial_record_visitor.hpp
        ${CMAKE_SOURCE_DIR}/include/job/job.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/console_mode.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/debug_draw_impl.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/game_mode.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/load_menu_mode.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/loading_menu_mode.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/main_menu_mode.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/menu_mode.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/menu_mode_base.hpp
        ${CMAKE_SOURCE_DIR}/include/modes/mode.hpp
        ${CMAKE_SOURCE_DIR}/include/persistent_reference_locator.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/acti_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/cell_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/cont_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/door_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/flor_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/furn_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/helpers.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/ligh_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/misc_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/npc__resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/resolvers.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/stat_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/resolvers/wrld_resolver.hpp
        ${CMAKE_SOURCE_DIR}/include/save_state.hpp
        ${CMAKE_SOURCE_DIR}/include/script_functions.hpp
        ${CMAKE_SOURCE_DIR}/include/wrld.hpp
        application.cpp
        application_context.cpp
        atmosphere.cpp
        audio.cpp
        cell_cache.cpp
        character_controller/animation.cpp
        character_controller/body.cpp
        character_controller/character.cpp
        character_controller/character_controller.cpp
        character_controller/character_controller_impl.cpp
        character_controller/jump_state.cpp
        character_controller/movement.cpp
        character_controller/run_state.cpp
        character_controller/sneak_jump_state.cpp
        character_controller/sneak_stand_state.cpp
        character_controller/stand_state.cpp
        character_controller/walk_state.cpp
        chrono.cpp
        console_functions.cpp
        exterior_manager.cpp
        initial_record_visitor.cpp
        main.cpp
        modes/console_mode.cpp
        modes/debug_draw_impl.cpp
        modes/game_mode.cpp
        modes/load_menu_mode.cpp
        modes/loading_menu_mode.cpp
        modes/main_menu_mode.cpp
        modes/menu_mode_base.cpp
        persistent_reference_locator.cpp
        resolvers/acti_resolver.cpp
        resolvers/cell_resolver.cpp
        resolvers/cont_resolver.cpp
        resolvers/door_resolver.cpp
        resolvers/flor_resolver.cpp
        resolvers/furn_resolver.cpp
        resolvers/helpers.cpp
        resolvers/ligh_resolver.cpp
        resolvers/misc_resolver.cpp
        resolvers/npc__resolver.cpp
        resolvers/stat_resolver.cpp
        resolvers/wrld_impl.cpp
        resolvers/wrld_impl.hpp
        resolvers/wrld_resolver.cpp
        save_state.cpp
        script_functions.cpp
        wrld.cpp)

target_link_libraries(OpenOBL PRIVATE
        Bullet::BulletCollision
        Bullet::BulletDynamics
        OpenOBL::OpenOBLBsa
        OpenOBL::OpenOBLBullet
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
        em::cppbitflag
        em::nostdx
        MicrosoftGSL::GSL
        OgreMain
        OgreGLSupport
        OgreOverlay
        OgreTerrain
        RenderSystem_GL3Plus
        SDL2::SDL2
        spdlog::spdlog
        optional)

if (MSVC)
    # TODO: Export symbols properly on MSVC
else ()
    target_link_options(OpenOBL PRIVATE -Wl,--dynamic-list=${CMAKE_SOURCE_DIR}/symbols.list)
    target_sources(OpenOBL PRIVATE ${CMAKE_SOURCE_DIR}/symbols.list)

    # TODO: Support libc++, probably by providing a OO_USE_LIBC++ option.
    target_link_libraries(OpenOBL PRIVATE SDL2::SDL2main stdc++fs)
endif ()

install(TARGETS OpenOBL
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})