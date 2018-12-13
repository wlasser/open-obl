cmake_minimum_required(VERSION 3.12)
project(OpenOblivionGui)

add_library(OpenOblivionGui STATIC)
target_compile_options(OpenOblivionGui PRIVATE -Wall)
target_compile_features(OpenOblivionGui PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionGui PROPERTY CXX_EXTENSION OFF)

add_library(OpenOblivion::OpenOblivionGui ALIAS OpenOblivionGui)

target_include_directories(OpenOblivionGui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionGui PRIVATE
        ${CMAKE_SOURCE_DIR}/include/gui/elements/image.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/elements/rect.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/elements/text.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/gui.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/logging.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/menu.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/menus/loading_menu.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/screen.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/stack/instructions.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/stack/program.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/stack/types.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/strings.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/trait.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/trait_selector.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/traits.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/ui_element.hpp
        ${CMAKE_SOURCE_DIR}/include/gui/xml.hpp
        elements/image.cpp
        gui.cpp
        screen.cpp
        stack/instructions.cpp
        stack/program.cpp
        stack/types.cpp
        strings.cpp
        trait.cpp
        trait_selector.cpp
        traits.cpp
        xml.cpp)

target_link_libraries(OpenOblivionGui PRIVATE
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionOgre
        Boost::boost
        spdlog::spdlog
        PUBLIC
        OgreOverlay
        pugixml)

install(TARGETS OpenOblivionGui EXPORT OpenOblivionGuiTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT OpenOblivionGuiTargets
        FILE OpenOblivionGuiTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)