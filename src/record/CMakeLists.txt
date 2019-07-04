cmake_minimum_required(VERSION 3.12)
project(OpenOBLRecord)

add_library(OpenOBLRecord STATIC)
add_library(OpenOBL::OpenOBLRecord ALIAS OpenOBLRecord)

target_include_directories(OpenOBLRecord PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_sources(OpenOBLRecord PRIVATE
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

target_link_libraries(OpenOBLRecord PRIVATE
        MicrosoftGSL::GSL
        ZLIB::ZLIB
        PUBLIC
        OpenOBL::OpenOBLIO
        em::cppbitflag
        Boost::boost)

install(TARGETS OpenOBLRecord EXPORT OpenOBLRecordTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT OpenOBLRecordTargets
        FILE OpenOBLRecordTargets.cmake
        NAMESPACE OpenOBL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenOBL)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/record/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenOBL/record)