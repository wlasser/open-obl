cmake_minimum_required(VERSION 3.12)
project(OpenOblivionOgreImGui)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionOgreImGui STATIC)

add_library(OpenOblivion::OpenOblivionOgreImGui ALIAS OpenOblivionOgreImGui)

target_include_directories(OpenOblivionOgreImGui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/imgui>
        $<INSTALL_INTERFACE:imgui>)

set(IMGUI_SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)

add_library(ImGui OBJECT
        ${IMGUI_SOURCE_DIR}/imgui/imconfig.h
        ${IMGUI_SOURCE_DIR}/imgui/imgui.cpp
        ${IMGUI_SOURCE_DIR}/imgui/imgui.h
        ${IMGUI_SOURCE_DIR}/imgui/imgui_demo.cpp
        ${IMGUI_SOURCE_DIR}/imgui/imgui_draw.cpp
        ${IMGUI_SOURCE_DIR}/imgui/imgui_internal.h
        ${IMGUI_SOURCE_DIR}/imgui/imgui_widgets.cpp
        ${IMGUI_SOURCE_DIR}/imgui/imstb_rectpack.h
        ${IMGUI_SOURCE_DIR}/imgui/imstb_textedit.h
        ${IMGUI_SOURCE_DIR}/imgui/imstb_truetype.h)

set_target_properties(ImGui PROPERTIES CXX_CLANG_TIDY "")

target_sources(OpenOblivionOgreImGui PRIVATE
        $<TARGET_OBJECTS:ImGui>
        ${CMAKE_SOURCE_DIR}/include/ogreimgui/imgui_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogreimgui/imgui_renderable.hpp
        imgui_manager.cpp
        imgui_renderable.cpp)

target_link_libraries(OpenOblivionOgreImGui PRIVATE
        OpenOblivion::OpenOblivionSDL
        PUBLIC
        OgreMain)

install(TARGETS OpenOblivionOgreImGui EXPORT OpenOblivionOgreImGuiTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionOgreImGuiTargets
        FILE OpenOblivionOgreImGuiTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
