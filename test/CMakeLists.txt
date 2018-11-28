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

target_sources(OpenOblivionTest PRIVATE
        ${CMAKE_SOURCE_DIR}/src/gui/stack/program.cpp
        ${CMAKE_SOURCE_DIR}/src/gui/stack/types.cpp
        ${CMAKE_SOURCE_DIR}/src/gui/trait.cpp
        ${CMAKE_SOURCE_DIR}/src/gui/trait_selector.cpp
        ${CMAKE_SOURCE_DIR}/src/gui/xml.cpp)

target_link_libraries(OpenOblivionTest PRIVATE
        OpenOblivion::OpenOblivionIO
        OpenOblivion::OpenOblivionFS
        OpenOblivion::OpenOblivionScripting
        Catch2::Catch2
        pugixml
        taocpp::pegtl)