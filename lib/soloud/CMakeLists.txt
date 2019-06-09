# This file is not part of the original SoLoud software.
cmake_minimum_required(VERSION 3.12)
project(SoLoud)

add_library(SoLoud STATIC)
if (MSVC)
    target_compile_options(SoLoud PRIVATE /W4)
else ()
    target_compile_options(SoLoud PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif ()
# SoLoud compiles with fno-rtti and fno-exceptions, but we use these features
# and mixing flags is a bad idea. SoLoud is also written in C++98, but again we
# shouldn't mix flags.
target_compile_features(SoLoud PUBLIC cxx_std_17)
if (MSVC)
    target_compile_definitions(SoLoud PUBLIC WITH_SDL2)
else ()
    target_compile_definitions(SoLoud PUBLIC WITH_SDL2_STATIC)
endif ()
set_property(TARGET SoLoud PROPERTY CXX_EXTENSION OFF)

add_library(SoLoud::SoLoud ALIAS SoLoud)

target_include_directories(SoLoud PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/soloud>
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>)

target_sources(SoLoud PRIVATE
        # We only need wav and mp3 support so the wav backend is sufficient
        src/audiosource/wav/dr_flac.h
        src/audiosource/wav/dr_impl.cpp
        src/audiosource/wav/dr_mp3.h
        src/audiosource/wav/dr_wav.h
        src/audiosource/wav/soloud_wav.cpp
        src/audiosource/wav/soloud_wavstream.cpp
        src/audiosource/wav/stb_vorbis.c
        src/audiosource/wav/stb_vorbis.h
        # Need all of core
        src/core/soloud.cpp
        src/core/soloud_audiosource.cpp
        src/core/soloud_bus.cpp
        src/core/soloud_core_3d.cpp
        src/core/soloud_core_basicops.cpp
        src/core/soloud_core_faderops.cpp
        src/core/soloud_core_filterops.cpp
        src/core/soloud_core_getters.cpp
        src/core/soloud_core_setters.cpp
        src/core/soloud_core_voicegroup.cpp
        src/core/soloud_core_voiceops.cpp
        src/core/soloud_fader.cpp
        src/core/soloud_fft.cpp
        src/core/soloud_fft_lut.cpp
        src/core/soloud_file.cpp
        src/core/soloud_filter.cpp
        src/core/soloud_queue.cpp
        src/core/soloud_thread.cpp
        # Need all of filter
        src/filter/soloud_bassboostfilter.cpp
        src/filter/soloud_biquadresonantfilter.cpp
        src/filter/soloud_dcremovalfilter.cpp
        src/filter/soloud_echofilter.cpp
        src/filter/soloud_fftfilter.cpp
        src/filter/soloud_flangerfilter.cpp
        src/filter/soloud_lofifilter.cpp
        src/filter/soloud_robotizefilter.cpp
        src/filter/soloud_waveshaperfilter.cpp
        # Only need the SDL2 Static backend, as mentioned
        src/backend/sdl2_static/soloud_sdl2_static.cpp)

target_link_libraries(SoLoud PUBLIC
        SDL2::SDL2
        SDL2::SDL2main)

install(TARGETS SoLoud EXPORT SoLoudTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT SoLoudTargets
        FILE SoLoudTargets.cmake
        NAMESPACE SoLoud::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SoLoud)