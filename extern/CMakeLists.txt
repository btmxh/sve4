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
endif()

# add_subdirectory(tinycthread) error due to outdated CMake
add_library(
    tinycthread
    tinycthread/source/tinycthread.c
    tinycthread/source/tinycthread.h
)

find_package(Threads REQUIRED)
target_include_directories(tinycthread PUBLIC tinycthread/source)
target_link_libraries(tinycthread PUBLIC Threads::Threads)
