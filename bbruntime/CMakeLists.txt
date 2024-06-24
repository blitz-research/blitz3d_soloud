cmake_minimum_required(VERSION 3.16)

project(bbruntime)

set(SRCS
	basic.cpp
	basic.h
	bbbank.cpp
	bbbank.h
	bbfilesystem.cpp
	bbfilesystem.h
	bbmath.cpp
	bbmath.h
	bbruntime.cpp
	bbruntime.h
	bbsockets.cpp
	bbsockets.h
	bbstream.cpp
	bbstream.h
	bbstring.cpp
	bbstring.h
	bbsys.cpp
	bbsys.h
	std.cpp
	std.h
	userlibs.cpp
	userlibs.h
)
set(LIBS gxruntime freetype)

if (BB_BLITZ3D_ENABLED)

	set(SRCS ${SRCS}
        bbinput.cpp
        bbinput.h
        bbblitz3d.cpp
        bbblitz3d.h
        bbgraphics.cpp
        bbgraphics.h
	)
    set(LIBS gxruntime blitz3d)

    if(BB_FMOD_ENABLED)
        set(SRCS ${SRCS}
            bbaudio.cpp
            bbaudio.h
        )
        set(LIBS ${LIBS} fmod375)
    else()
        set(SRCS ${SRCS}
           bbaudio_soloud.cpp
           bbaudio_soloud.h
        )
        set(LIBS ${LIBS} soloud)
    endif()

elseif(BB_LIBSGD_ENABLED)

	set(SRCS ${SRCS}
		bbsgd.cpp
		bbsgd.h
	)
	set(LIBS gxruntime libsgd)

endif()

add_library(bbruntime ${SRCS})

target_compile_options(bbruntime PRIVATE /Gz)

target_link_libraries(bbruntime ${LIBS})
