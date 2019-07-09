cmake_minimum_required(VERSION 3.12)
project(OpenOBLEspTools)

include(GNUInstallDirs)

add_executable(OpenOBLEspEditor)
set_property(TARGET OpenOBLEspEditor PROPERTY OUTPUT_NAME espedit)

target_include_directories(OpenOBLEspEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(OpenOBLEspEditor PRIVATE
        application.hpp
        application.cpp
        esp_editor.cpp)

target_link_libraries(OpenOBLEspEditor PRIVATE
        OpenOBL::OpenOBLEsp
        OpenOBL::OpenOBLJob
        OpenOBL::OpenOBLOgre
        OpenOBL::OpenOBLOgreImGui
        OpenOBL::OpenOBLSDL
        OpenOBL::OpenOBLUtil
        RenderSystem_GL3Plus)

install(TARGETS OpenOBLEspEditor
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})