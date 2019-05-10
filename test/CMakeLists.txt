cmake_minimum_required(VERSION 3.12)
project(OpenOblivionTest)

add_executable(OpenOblivionTest tests.cpp)
target_compile_options(OpenOblivionTest PRIVATE -Wall)
target_compile_features(OpenOblivionTest PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionTest PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOblivionTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(fs)
add_subdirectory(gui)
add_subdirectory(io)
add_subdirectory(scripting)

target_sources(OpenOblivionTest PRIVATE chrono.cpp meta.cpp
        ${CMAKE_SOURCE_DIR}/src/game_settings.cpp
        ${CMAKE_SOURCE_DIR}/src/globals.cpp
        ${CMAKE_SOURCE_DIR}/src/chrono.cpp)

target_link_libraries(OpenOblivionTest PRIVATE
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionGui
        OpenOblivion::OpenOblivionIO
        OpenOblivion::OpenOblivionOgre
        OpenOblivion::OpenOblivionScripting
        Catch2::Catch2
        GSL
        taocpp::pegtl
        tloptional)

find_package(Threads REQUIRED)

add_executable(OpenOblivionJobsTest jobs.cpp)
target_compile_options(OpenOblivionJobsTest PRIVATE -Wall)
target_compile_features(OpenOblivionJobsTest PUBLIC cxx_std_17)
set_property(TARGET OpenOblivionJobsTest PROPERTY CXX_EXTENSION OFF)

target_include_directories(OpenOblivionJobsTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_sources(OpenOblivionJobsTest PRIVATE jobs.cpp)

target_link_libraries(OpenOblivionJobsTest
        Threads::Threads
        Boost::boost
        Boost::fiber)
