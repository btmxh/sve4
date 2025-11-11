if(BUILD_TESTING)
    add_library(
        munit
        STATIC
        munit/munit.c
        munit/munit.h
    )
    target_include_directories(munit PUBLIC munit)
    set_target_properties(
        munit
        PROPERTIES
            C_STANDARD_REQUIRED
                YES
    )
    if(MSVC)
        set_target_properties(
            munit
            PROPERTIES
                C_STANDARD
                    99
        )
    else()
        set_target_properties(
            munit
            PROPERTIES
                C_STANDARD
                    11
        )
    endif()
    set_property(
        TARGET
            munit
        PROPERTY
            POSITION_INDEPENDENT_CODE
                True
    )
endif()

# add_subdirectory(tinycthread) error due to outdated CMake
add_library(
    tinycthread
    STATIC
    tinycthread/source/tinycthread.c
    tinycthread/source/tinycthread.h
)

find_package(Threads REQUIRED)
target_include_directories(tinycthread PUBLIC tinycthread/source)
target_link_libraries(tinycthread PUBLIC Threads::Threads)

add_library(arena INTERFACE)
target_include_directories(arena INTERFACE .)

set(CIVETWEB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(civetweb)
