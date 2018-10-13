cmake_minimum_required(VERSION 3.12)
project(OpenOblivionRecord)

set(CMAKE_CXX_STANDARD 17)

add_library(OpenOblivionRecord STATIC)

add_library(OpenOblivion::OpenOblivionRecord ALIAS OpenOblivionRecord)

target_include_directories(OpenOblivionRecord PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionRecord PRIVATE
        ${CMAKE_SOURCE_DIR}/include/record/exceptions.hpp
        ${CMAKE_SOURCE_DIR}/include/record/definition_helpers.hpp
        ${CMAKE_SOURCE_DIR}/include/record/group.hpp
        ${CMAKE_SOURCE_DIR}/include/record/io.hpp
        ${CMAKE_SOURCE_DIR}/include/record/rec_of.hpp
        ${CMAKE_SOURCE_DIR}/include/record/record.hpp
        ${CMAKE_SOURCE_DIR}/include/record/record_header.hpp
        ${CMAKE_SOURCE_DIR}/include/record/size_of.hpp
        ${CMAKE_SOURCE_DIR}/include/record/subrecord.hpp
        ${CMAKE_SOURCE_DIR}/include/record/tuplifiable.hpp
        group.cpp
        io.cpp)

target_link_libraries(OpenOblivionRecord PRIVATE
        Boost::boost
        OpenOblivion::OpenOblivionIO)

install(TARGETS OpenOblivionRecord EXPORT OpenOblivionRecordTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionRecordTargets
        FILE OpenOblivionRecordTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
