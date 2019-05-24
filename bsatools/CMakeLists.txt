cmake_minimum_required(VERSION 3.12)
project(OpenOBLBsaTools)

find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)

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

        add_executable(OpenOBLBsaBrowser)
        target_compile_options(OpenOBLBsaBrowser PRIVATE -Wall)
        target_compile_features(OpenOBLBsaBrowser PUBLIC cxx_std_17)
        set_property(TARGET OpenOBLBsaBrowser PROPERTY CXX_EXTENSION OFF)

        target_include_directories(OpenOBLBsaBrowser PRIVATE ${CMAKE_SOURCE_DIR}/include)
        target_sources(OpenOBLBsaBrowser PRIVATE
                browser_application.hpp
                browser_application.cpp
                bsa_browser.cpp
                resources/resources.c)

        target_include_directories(OpenOBLBsaBrowser PRIVATE
                ${GTKMM_INCLUDE_DIRS})
        target_link_libraries(OpenOBLBsaBrowser PRIVATE
                OpenOBL::OpenOBLBsa
                OpenOBL::OpenOBLIO
                ${GTKMM_LIBRARIES}
                GSL)
    endif ()

    # BsaFuse FUSE application
    if (FUSE_FOUND)
        add_executable(OpenOBLBsaFuse)
        target_compile_options(OpenOBLBsaFuse PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_features(OpenOBLBsaFuse PUBLIC cxx_std_17)
        set_property(TARGET OpenOBLBsaFuse PROPERTY CXX_EXTENSION OFF)
        set_property(TARGET OpenOBLBsaFuse PROPERTY OUTPUT_NAME bsafuse)

        target_include_directories(OpenOBLBsaFuse PRIVATE ${CMAKE_SOURCE_DIR}/include)
        target_sources(OpenOBLBsaFuse PRIVATE
                bsa_fuse.hpp
                fuse.hpp
                fuse_operations.hpp
                fuse_options.hpp
                bsa_fuse.cpp
                fuse_operations.cpp
                fuse_options.cpp)

        target_include_directories(OpenOBLBsaFuse PRIVATE
                ${FUSE_INCLUDE_DIRS})
        target_link_libraries(OpenOBLBsaFuse PRIVATE
                OpenOBL::OpenOBLBsa
                OpenOBL::OpenOBLFS
                ${FUSE_LIBRARIES})

        install(TARGETS OpenOBLBsaFuse
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif ()
endif ()
