cmake_minimum_required(VERSION 3.12)
project(OpenOblivionRecord)

add_library(OpenOblivionRecord STATIC)
add_library(OpenOblivion::OpenOblivionRecord ALIAS OpenOblivionRecord)

target_include_directories(OpenOblivionRecord PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOblivionRecord PRIVATE
        ${CMAKE_SOURCE_DIR}/include/record/actor_value.hpp
        ${CMAKE_SOURCE_DIR}/include/record/attribute.hpp
        ${CMAKE_SOURCE_DIR}/include/record/change_record.hpp
        ${CMAKE_SOURCE_DIR}/include/record/definition_helpers.hpp
        ${CMAKE_SOURCE_DIR}/include/record/exceptions.hpp
        ${CMAKE_SOURCE_DIR}/include/record/formid.hpp
        ${CMAKE_SOURCE_DIR}/include/record/group.hpp
        ${CMAKE_SOURCE_DIR}/include/record/io.hpp
        ${CMAKE_SOURCE_DIR}/include/record/magic_effects.hpp
        ${CMAKE_SOURCE_DIR}/include/record/rec_of.hpp
        ${CMAKE_SOURCE_DIR}/include/record/record.hpp
        ${CMAKE_SOURCE_DIR}/include/record/record_header.hpp
        ${CMAKE_SOURCE_DIR}/include/record/records.hpp
        ${CMAKE_SOURCE_DIR}/include/record/records_fwd.hpp
        ${CMAKE_SOURCE_DIR}/include/record/reference_records.hpp
        ${CMAKE_SOURCE_DIR}/include/record/size_of.hpp
        ${CMAKE_SOURCE_DIR}/include/record/subrecord.hpp
        ${CMAKE_SOURCE_DIR}/include/record/subrecords.hpp
        ${CMAKE_SOURCE_DIR}/include/record/tuplifiable.hpp
        formid.cpp
        group.cpp
        io.cpp
        records.cpp
        subrecords.cpp)

target_link_libraries(OpenOblivionRecord PRIVATE
        MicrosoftGSL::GSL
        ZLIB::ZLIB
        PUBLIC
        OpenOblivion::OpenOblivionIO
        Boost::boost)

install(TARGETS OpenOblivionRecord EXPORT OpenOblivionRecordTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOblivionRecordTargets
        FILE OpenOblivionRecordTargets.cmake
        NAMESPACE OpenOblivion::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOblivion)
