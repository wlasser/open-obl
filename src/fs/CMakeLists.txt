cmake_minimum_required(VERSION 3.12)
project(OpenOBLFS)

add_library(OpenOBLFS STATIC)
add_library(OpenOBL::OpenOBLFS ALIAS OpenOBLFS)

target_include_directories(OpenOBLFS PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLFS PRIVATE
        ${CMAKE_SOURCE_DIR}/include/fs/path.hpp
        path.cpp)

target_link_libraries(OpenOBLFS PUBLIC
        OpenOBL::OpenOBLUtil
        Boost::fiber)

# TODO: Support libc++, probably by providing a OO_USE_LIBC++ option.
if (NOT MSVC)
    target_link_libraries(OpenOBLFS INTERFACE stdc++fs)
endif ()

install(TARGETS OpenOBLFS EXPORT OpenOBLFSTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLFSTargets
        FILE OpenOBLFSTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/fs/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/fs)
