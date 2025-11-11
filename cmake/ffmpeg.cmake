# These are the FFmpeg components we care about
set(FFmpeg_COMPONENTS
    AVCODEC
    AVFORMAT
    AVUTIL
    SWSCALE
    SWRESAMPLE
)

# first approach: use vcpkg or any package manager that provides a
# FindFFmpeg.cmake module
find_package(FFMPEG QUIET)
if(FFMPEG_FOUND)
    message(
        STATUS
        "FFmpeg found via find_package. Make sure that your installation
        contains all of the necessary components."
    )
    add_library(FFmpeg_FFmpeg INTERFACE IMPORTED)
    target_include_directories(FFmpeg_FFmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
    target_link_directories(FFmpeg_FFmpeg INTERFACE ${FFMPEG_LIBRARY_DIRS})
    target_link_libraries(FFmpeg_FFmpeg INTERFACE ${FFMPEG_LIBRARIES})

    if(UNIX AND NOT APPLE)
        target_link_options(FFmpeg_FFmpeg INTERFACE "-Wl,-Bsymbolic")
    endif()

    add_library(FFmpeg::FFmpeg ALIAS FFmpeg_FFmpeg)
    set(FFmpeg_FOUND ON)
    foreach(_component ${FFmpeg_COMPONENTS})
        add_library(FFmpeg_${_component} ALIAS FFmpeg_FFmpeg)
        add_library(FFmpeg::${_component} ALIAS FFmpeg_FFmpeg)
        set(FFmpeg_${_component}_FOUND TRUE)
    endforeach()
    set(FFmpeg_COMPONENTS "")
endif()

# first approach: use pkg-config
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    foreach(_component ${FFmpeg_COMPONENTS})
        string(TOLOWER ${_component} _component_lc)
        pkg_check_modules(FFmpeg_${_component} QUIET lib${_component_lc})

        if(NOT FFmpeg_${_component}_FOUND)
            continue()
        endif()

        add_library(FFmpeg_${_component} INTERFACE IMPORTED)

        message(STATUS "FFmpeg component found via pkg-config: ${_component}")

        if(DEFINED FFmpeg_${_component}_INCLUDE_DIRS)
            target_include_directories(
                FFmpeg_${_component}
                INTERFACE
                    ${FFmpeg_${_component}_INCLUDE_DIRS}
            )
        endif()

        if(DEFINED FFmpeg_${_component}_LIBRARY_DIRS)
            target_link_directories(
                FFmpeg_${_component}
                INTERFACE
                    ${FFmpeg_${_component}_LIBRARY_DIRS}
            )
        endif()

        if(DEFINED FFmpeg_${_component}_LIBRARIES)
            target_link_libraries(
                FFmpeg_${_component}
                INTERFACE
                    ${FFmpeg_${_component}_LIBRARIES}
            )
        endif()

        if(DEFINED FFmpeg_${_component}_LINKER_FLAGS)
            target_link_options(
                FFmpeg_${_component}
                INTERFACE
                    ${FFmpeg_${_component}_LINKER_FLAGS}
            )
        endif()

        add_library(FFmpeg::${_component} ALIAS FFmpeg_${_component})

        list(REMOVE_ITEM FFmpeg_COMPONENTS ${_component})
        set(FFmpeg_${_component}_FOUND TRUE)
    endforeach()
endif()

# second approach: use find_library/find_path
foreach(_component ${FFmpeg_COMPONENTS})
    string(TOLOWER ${_component} _component_lc)

    find_path(
        FFmpeg_${_component}_INCLUDE_DIRS
        NAMES
            # this common scheme fails for libpostproc, but we don't care about
            # it
            lib${_component_lc}/lib${_component_lc}.h
        HINTS
        ENV FFMPEG_INCLUDE_DIR
    )

    find_library(
        FFmpeg_${_component}_LIBRARIES
        NAMES
            av${_component_lc}
        HINTS
        ENV FFMPEG_LIBRARY_DIR
    )

    if(
        FFmpeg_${_component}_INCLUDE_DIRS
            STREQUAL
            "FFmpeg_${_component}-NOTFOUND"
    )
        continue()
    endif()

    if(FFmpeg_${_component}_LIBRARIES STREQUAL "FFmpeg_${_component}-NOTFOUND")
        continue()
    endif()

    message(
        STATUS
        "FFmpeg component found via find_path/find_library: ${_component}"
    )

    add_library(FFmpeg_${_component} INTERFACE IMPORTED)

    target_include_directories(
        FFmpeg_${_component}
        INTERFACE
            ${FFmpeg_${_component}_INCLUDE_DIRS}
    )

    target_link_libraries(
        FFmpeg_${_component}
        INTERFACE
            ${FFmpeg_${_component}_LIBRARIES}
    )

    add_library(FFmpeg::${_component} ALIAS FFmpeg_${_component})

    list(REMOVE_ITEM FFmpeg_COMPONENTS ${_component})
    set(FFmpeg_${_component}_FOUND TRUE)
endforeach()
