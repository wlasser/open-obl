cmake_minimum_required(VERSION 3.12)
project(OpenOBLGui)

add_library(OpenOBLGui STATIC)
add_library(OpenOBL::OpenOBLGui ALIAS OpenOBLGui)

target_include_directories(OpenOBLGui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_include_directories(OpenOBLGui PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_sources(OpenOBLGui PRIVATE
        ${CMAKE_SOURCE_DIR}/include/gui/elements/button.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/elements/generic_background.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/elements/image.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/elements/load_game_entry.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/elements/rect.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/elements/text.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/elements/vertical_scroll.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/gui.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/logging.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/menu.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/menus/load_menu.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/menus/loading_menu.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/menus/main_menu.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/sound.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/trait.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/ui_element.hpp
        elements/image.cpp
        elements/interactable_mixin.cpp
        elements/panel_mixin.cpp
        elements/rect.cpp
        elements/text.cpp
        gui.cpp
        gui_impl.cpp
        gui_impl.hpp
        menus/load_menu.cpp
        menus/loading_menu.cpp
        menus/main_menu.cpp
        screen.cpp
        screen.hpp
        sound.cpp
        stack/instructions.cpp
        stack/instructions.hpp
        stack/program.cpp
        stack/program.hpp
        stack/types.cpp
        stack/types.hpp
        strings.cpp
        strings.hpp
        trait.cpp
        trait_selector.cpp
        trait_selector.hpp
        traits.cpp
        traits.hpp
        xml.cpp
        xml.hpp)

target_link_libraries(OpenOBLGui PRIVATE
        OpenOBL::OpenOBLConfig
        OpenOBL::OpenOBLFS
        OpenOBL::OpenOBLOgre
        Boost::boost
        Boost::fiber
        pugixml
        PUBLIC
        OpenOBL::OpenOBLUtil
        em::nostdx
        OgreOverlay
        spdlog::spdlog)

install(TARGETS OpenOBLGui EXPORT OpenOBLGuiTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLGuiTargets
        FILE OpenOBLGuiTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/gui/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/gui)