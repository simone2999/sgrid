option(SGRID_ENABLE_DEV_MODE
       "Add additional flags for more strict compilation" ON)


if(SGRID_ENABLE_DEV_MODE)
    set(SGRID_DEV_FLAGS
        "-Wall -Wextra -pedantic -Werror -Werror=enum-compare -Werror=delete-non-virtual-dtor -Werror=reorder -Werror=return-type" # -Werror=uninitialized
    )
endif()

if(NOT CMAKE_BUILD_TYPE)

    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "Choose the type of build, options are: Debug Release
RelWithDebInfo MinSizeRel." FORCE)

    message(STATUS "[Status] CMAKE_BUILD_TYPE=Release")

endif(NOT CMAKE_BUILD_TYPE)




set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${SGRID_DEV_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SGRID_DEV_FLAGS}")