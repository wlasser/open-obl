cmake_minimum_required(VERSION 3.12)
project(OpenOBLTest)

add_executable(OpenOBLTest tests.cpp)
if (MSVC)
    target_compile_options(OpenOBLTest PRIVATE /W4)
else ()
    target_compile_options(OpenOBLTest PRIVATE -Wall)
endif ()
target_compile_features(OpenOBLTest PUBLIC cxx_std_17)
set_property(TARGET OpenOBLTest PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOBLTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(fs)
add_subdirectory(gui)
add_subdirectory(io)
add_subdirectory(scripting)

target_sources(OpenOBLTest PRIVATE chrono.cpp meta.cpp
        ${CMAKE_SOURCE_DIR}/src/chrono.cpp)

target_link_libraries(OpenOBLTest PRIVATE
        OpenOBL::OpenOBLConfig
        OpenOBL::OpenOBLFS
        OpenOBL::OpenOBLGui
        OpenOBL::OpenOBLIO
        OpenOBL::OpenOBLOgre
        OpenOBL::OpenOBLScripting
        OpenOBL::OpenOBLUtil
        Catch2::Catch2
        MicrosoftGSL::GSL
        taocpp::pegtl
        tloptional)

find_package(Threads REQUIRED)

add_executable(OpenOBLJobsTest jobs.cpp)
if (MSVC)
    target_compile_options(OpenOBLJobsTest PRIVATE /W4)
else ()
    target_compile_options(OpenOBLJobsTest PRIVATE -Wall)
endif ()
target_compile_features(OpenOBLJobsTest PUBLIC cxx_std_17)
set_property(TARGET OpenOBLJobsTest PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOBLJobsTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_sources(OpenOBLJobsTest PRIVATE jobs.cpp)

target_link_libraries(OpenOBLJobsTest
        Threads::Threads
        Boost::boost
        Boost::fiber)
