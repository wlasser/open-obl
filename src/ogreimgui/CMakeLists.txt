cmake_minimum_required(VERSION 3.12)
project(OpenOBLOgreImGui)

add_library(OpenOBLOgreImGui STATIC)
add_library(OpenOBL::OpenOBLOgreImGui ALIAS OpenOBLOgreImGui)

target_include_directories(OpenOBLOgreImGui PUBLIC
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

target_sources(OpenOBLOgreImGui PRIVATE
        $<TARGET_OBJECTS:ImGui>
        ${CMAKE_SOURCE_DIR}/include/ogreimgui/imgui_manager.hpp
        ${CMAKE_SOURCE_DIR}/include/ogreimgui/imgui_renderable.hpp
        imgui_manager.cpp
        imgui_renderable.cpp)

target_link_libraries(OpenOBLOgreImGui PRIVATE
        OpenOBL::OpenOBLSDL
        PUBLIC
        OgreMain)

install(TARGETS OpenOBLOgreImGui EXPORT OpenOBLOgreImGuiTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLOgreImGuiTargets
        FILE OpenOBLOgreImGuiTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ogreimgui/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/ogreimgui)

install(FILES ${IMGUI_SOURCE_DIR}/imgui/imgui.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/imgui/)