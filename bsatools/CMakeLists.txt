cmake_minimum_required(VERSION 3.12)
project(OpenOblivionBsaTools)

find_package(PkgConfig)
if (PkgConfig_FOUND)

    pkg_check_modules(GTKMM gtkmm-3.0)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources)

    add_custom_command(OUTPUT resources/resources.c
            COMMAND glib-compile-resources --target=${CMAKE_CURRENT_BINARY_DIR}/resources/resources.c --generate-source bsabrowser.gresource.xml
            DEPENDS resources/bsabrowser.gresource.xml resources/window.glade
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources)

    add_executable(OpenOblivionBsaBrowser)
    target_compile_options(OpenOblivionBsaBrowser PRIVATE -Wall)
    target_compile_features(OpenOblivionBsaBrowser PUBLIC cxx_std_17)
    set_property(TARGET OpenOblivionBsaBrowser PROPERTY CXX_EXTENSION OFF)
    target_include_directories(OpenOblivionBsaBrowser PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_sources(OpenOblivionBsaBrowser PRIVATE
            application.hpp
            application.cpp
            main.cpp
            resources/resources.c)

    target_include_directories(OpenOblivionBsaBrowser PRIVATE
            ${GTKMM_INCLUDE_DIRS})
    target_link_libraries(OpenOblivionBsaBrowser PRIVATE
            OpenOblivion::OpenOblivionBsa
            OpenOblivion::OpenOblivionIO
            ${GTKMM_LIBRARIES}
            GSL)
endif ()
