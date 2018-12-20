cmake_minimum_required(VERSION 3.12)
project(OpenOblivionBsaTools)

find_package(PkgConfig REQUIRED)

if (PkgConfig_FOUND)
    pkg_check_modules(GTKMM gtkmm-3.0)
    pkg_check_modules(FUSE fuse>=2.9.8)

    # BsaBrowser GTK application
    if (GTKMM_FOUND)
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
                browser_application.hpp
                browser_application.cpp
                bsa_browser.cpp
                resources/resources.c)

        target_include_directories(OpenOblivionBsaBrowser PRIVATE
                ${GTKMM_INCLUDE_DIRS})
        target_link_libraries(OpenOblivionBsaBrowser PRIVATE
                OpenOblivion::OpenOblivionBsa
                OpenOblivion::OpenOblivionIO
                ${GTKMM_LIBRARIES}
                GSL)
    endif ()

    # BsaFuse FUSE application
    if (FUSE_FOUND)
        add_executable(OpenOblivionBsaFuse)
        target_compile_options(OpenOblivionBsaFuse PRIVATE -Wall)
        target_compile_features(OpenOblivionBsaFuse PUBLIC cxx_std_17)
        set_property(TARGET OpenOblivionBsaFuse PROPERTY CXX_EXTENSION OFF)

        target_include_directories(OpenOblivionBsaFuse PRIVATE ${CMAKE_SOURCE_DIR}/include)
        target_sources(OpenOblivionBsaFuse PRIVATE
                bsa_fuse.cpp)

        target_include_directories(OpenOblivionBsaFuse PRIVATE
                ${FUSE_INCLUDE_DIRS})
        target_link_libraries(OpenOblivionBsaFuse PRIVATE
                OpenOblivion::OpenOblivionIO
                ${FUSE_LIBRARIES})
    endif ()
endif ()
