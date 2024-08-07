cmake_minimum_required(VERSION 3.16)

project("libsgd")

add_library(libsgd INTERFACE)

# Assumes libsgd repos is in same dir as blitz3d, and cmake-build-windows-x86-release/libsgd/sgd_static.lib target has already been built.
# What could possibly go wrong?!

set(LIBSGD_DIR ${CMAKE_SOURCE_DIR}/../libsgd)

target_include_directories(libsgd INTERFACE ${LIBSGD_DIR}/libsgd/include)

set(LIBS
	dxguid
	crypt32.lib
	Wldap32.lib
	Ws2_32.lib
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(LIBS ${LIBS} ${LIBSGD_DIR}/cmake-build-windows-x86-debug/libsgd/sgd_static.lib)
else ()
	set(LIBS ${LIBS} ${LIBSGD_DIR}/cmake-build-windows-x86-release/libsgd/sgd_static.lib)
endif ()

target_link_libraries(libsgd INTERFACE ${LIBS})
