cmake_minimum_required(VERSION 3.16)

project(soloud)

include(FetchContent)

##### soloud #####

FetchContent_Declare(soloud
        GIT_REPOSITORY https://github.com/blitz-research/soloud.git
        GIT_TAG master
        GIT_SUBMODULES ""
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(soloud)

set(CORE ${soloud_SOURCE_DIR}/src/core)
set(BACKEND ${soloud_SOURCE_DIR}/src/backend)
set(ASRC ${soloud_SOURCE_DIR}/src/audiosource)

set(SRCS
        ${CORE}/soloud.cpp
        ${CORE}/soloud_core_basicops.cpp
        ${CORE}/soloud_core_setters.cpp
        ${CORE}/soloud_fft.cpp
        ${CORE}/soloud_misc.cpp
        ${CORE}/soloud_audiosource.cpp
        ${CORE}/soloud_core_faderops.cpp
        ${CORE}/soloud_core_voicegroup.cpp
        ${CORE}/soloud_fft_lut.cpp
        ${CORE}/soloud_queue.cpp
        ${CORE}/soloud_bus.cpp
        ${CORE}/soloud_core_filterops.cpp
        ${CORE}/soloud_core_voiceops.cpp
        ${CORE}/soloud_file.cpp
        ${CORE}/soloud_thread.cpp
        ${CORE}/soloud_core_3d.cpp
        ${CORE}/soloud_core_getters.cpp
        ${CORE}/soloud_fader.cpp
        ${CORE}/soloud_filter.cpp
        ${ASRC}/wav/soloud_wav.cpp
        ${ASRC}/wav/soloud_wavstream.cpp
        ${ASRC}/wav/stb_vorbis.c
        ${ASRC}/wav/dr_impl.cpp
)

set(SRCS ${SRCS}
        ${BACKEND}/xaudio2/soloud_xaudio2.cpp
        ${BACKEND}/winmm/soloud_winmm.cpp
)
set(WITH_BACKENDS WITH_XAUDIO2=1 WITH_WINMM=1)

add_library(soloud ${SRCS})

target_compile_definitions(soloud PUBLIC ${WITH_BACKENDS})

target_include_directories(soloud PUBLIC ${soloud_SOURCE_DIR}/include)

target_link_libraries(soloud ${LIBS})
