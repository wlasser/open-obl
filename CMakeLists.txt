cmake_minimum_required(VERSION 3.13)
project(OpenOBL)

include(CheckIPOSupported)
include(FetchContent)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (MSVC)
    add_compile_options(/Zc:__cplusplus)
endif ()

option(OO_USE_SHARED_LLVM_LIBS "Link against LLVM shared libraries" OFF)
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
        if (MSVC)
            set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE STRING "Use MSVC Runtime Library DLL (/MD or /MDd)")
            set(USE_MSVC_DISABLE_RTTI OFF CACHE STRING "Disable RTTI on MSVC (/GR- or /GR)")
        endif ()
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
find_package(OGRE 1.12.1 REQUIRED COMPONENTS Overlay Paging Terrain)
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
if (MSVC)
    add_compile_options(/W4 /w14640)
else ()
    add_compile_options(-Wall -Wextra -Wshadow -Wpedantic)
endif ()

add_subdirectory(src/bsa)
add_subdirectory(src/bullet)
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
add_subdirectory(src)
add_subdirectory(test)
