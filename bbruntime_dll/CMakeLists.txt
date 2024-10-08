cmake_minimum_required(VERSION 3.16)

project(bbruntime_dll)

add_library(bbruntime_dll MODULE
	bbexe.ico
	bbruntime_dll.cpp
	bbruntime_dll.h
	bbruntime_dll.rc
	resource.h)

target_compile_options(bbruntime_dll PRIVATE /Gz)

target_link_options(bbruntime_dll PRIVATE /SUBSYSTEM:WINDOWS)

target_link_libraries(bbruntime_dll bbruntime)

copy_dll_to_install(bbruntime_dll bin/runtime)

if (BB_LIBSGD_ENABLED)
	# Very hacky. Also copy output runtime.dll to remote LIBSGD_INSTALL/blitz3d dir.
	set(SRC $<TARGET_FILE_DIR:bbruntime_dll>/bbruntime_dll.dll)
	set(DST ${CMAKE_SOURCE_DIR}/../libsgd/cmake-build-windows-x64-release/LIBSGD_INSTALL/blitz3d/runtime.dll)
	add_custom_command(TARGET bbruntime_dll POST_BUILD VERBATIM
		COMMAND ${CMAKE_COMMAND} -E echo "Copying bbruntime_dll output file \"${SRC}\" to \"${DST}\""
		COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST})
endif()

if (BB_FMOD_ENABLED)
	set(SRC ${CMAKE_SOURCE_DIR}/fmod375/lib/fmod.dll)
	set(DST ${CMAKE_SOURCE_DIR}/BLITZ3D_INSTALL/bin/fmod.dll)
	add_custom_command(TARGET bbruntime_dll POST_BUILD VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo "Copying \"${SRC}\" to \"${DST}\""
	COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST})
else()
	set(DST ${CMAKE_SOURCE_DIR}/BLITZ3D_INSTALL/bin/fmod.dll)
	add_custom_command(TARGET bbruntime_dll POST_BUILD VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo "Removing \"${DST}\""
	COMMAND ${CMAKE_COMMAND} -E rm -f ${DST})
endif()
