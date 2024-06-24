cmake_minimum_required(VERSION 3.16)

# Set ON for some debugging
set(CMAKE_VERBOSE_MAKEFILE OFF)

project("Blitz3D")

option(BB_LIBSGD_ENABLED "Blitz3D LibSGD build enabled" OFF)
option(BB_FMOD_ENABLED "Blitz3D FMOD build enabled" OFF)

if (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	message(FATAL_ERROR "Blitz3D must be built using MSVC.")
endif ()

if (NOT (CMAKE_SIZEOF_VOID_P EQUAL 4))
	message(FATAL_ERROR "Blitz3D must be built in 32 bit mode: Pass '-DCMAKE_GENERATOR_PLATFORM=Win32' to cmake when configuring.")
endif ()

if(BB_LIBSGD_ENABLED AND BB_FMOD_ENABLED)
	message(FATAL_ERROR "Blitz3D FMOD builds not available in LibSGD builds.")
endif()

message(STATUS "### Configuring Blitz3D build for build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "### BB_LIBSGD_ENABLED: ${BB_LIBSGD_ENABLED}")
message(STATUS "### BB_FMOD_ENABLED: ${BB_FMOD_ENABLED}")

if(BB_LIBSGD_ENABLED)
	add_compile_definitions(BB_LIBSGD_ENABLED=1)
else()
	set(BB_BLITZ3D_ENABLED ON)
	add_compile_definitions(BB_BLITZ3D_ENABLED=1)
	if(BB_FMOD_ENABLED)
		add_compile_definitions(BB_FMOD_ENABLED=1)
	endif()
endif()

# Using C++ 14
set(CMAKE_CXX_STANDARD 14)

# Add our root include directory
include_directories(${CMAKE_SOURCE_DIR})

# Use static CRT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

function(copy_dll_to_install target dst)
	set(SRC $<TARGET_FILE_DIR:${target}>/${target}.dll)
	set(DST ${CMAKE_SOURCE_DIR}/BLITZ3D_INSTALL/${dst}.dll)
	add_custom_command(TARGET ${target} POST_BUILD VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo "Copying ${target} output file \"${SRC}\" to \"${DST}\""
			COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST})
endfunction()

function(copy_exe_to_install target dst)
	set(SRC $<TARGET_FILE_DIR:${target}>/${target}.exe)
	set(DST ${CMAKE_SOURCE_DIR}/BLITZ3D_INSTALL/${dst}.exe)
	add_custom_command(TARGET ${target} POST_BUILD VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo "Copying ${target} output file \"${SRC}\" to \"${DST}\""
			COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST})
endfunction()

add_subdirectory(config)
add_subdirectory(stdutil)
add_subdirectory(compiler)
add_subdirectory(blitz)
add_subdirectory(linker)
add_subdirectory(linker_dll)
add_subdirectory(debugger)
add_subdirectory(blitzide)
add_subdirectory(freetype)
add_subdirectory(gxruntime)
add_subdirectory(bbruntime)
add_subdirectory(bbruntime_dll)
add_subdirectory(bblaunch)

if (BB_BLITZ3D_ENABLED)
	add_subdirectory(freeimage)
	add_subdirectory(blitz3D)
    if (BB_FMOD_ENABLED)
        add_subdirectory(fmod375)
    else()
        add_subdirectory(soloud)
    endif()
elseif(BB_LIBSGD_ENABLED)
	add_subdirectory(libsgd)
endif ()
