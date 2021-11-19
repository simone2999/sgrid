cmake_minimum_required(VERSION 3.5...3.19)
cmake_policy(SET CMP0048 NEW)

project(
    main
    VERSION 0.0.1
    LANGUAGES "CXX"
    HOMEPAGE_URL "https://bitbucket.org/zulianp/sgrid/"
    DESCRIPTION
        "Structured grid rid manager"
)

include(CMakePackageConfigHelpers)

include(cmake/SGridDefaults.cmake)
include(cmake/SGridDependencies.cmake)
include(cmake/SGridFunctions.cmake)
include(cmake/SGridOptions.cmake)

# ##############################################################################
# ##############################################################################
# Library
# ##############################################################################

set(SGRID_MODULES core)

foreach(MODULE ${SGRID_MODULES})
    add_subdirectory("${MODULE}")
endforeach(MODULE)

message(STATUS "HERE:\n${SGRID_SOURCES}\n
              ${SGRID_HEADERS}\n ${SGRID_BUILD_INCLUDES}\n
              SGRID_DEP_INCLUDES=${SGRID_DEP_INCLUDES}\n
              SGRID_DEP_LIBRARIES=${SGRID_DEP_LIBRARIES}
              ")

add_library(sgrid ${SGRID_SOURCES} ${SGRID_HEADERS})

foreach(include ${SGRID_BUILD_INCLUDES})
    target_include_directories(sgrid BEFORE
                               PUBLIC $<BUILD_INTERFACE:${include}>)
endforeach()

target_include_directories(sgrid SYSTEM PUBLIC ${SGRID_DEP_INCLUDES})
target_link_libraries(sgrid PUBLIC ${SGRID_DEP_LIBRARIES})

# ##############################################################################

add_executable(example_1 drivers/example_1.cpp)
target_link_libraries(example_1 PUBLIC sgrid)

add_executable(example_2 drivers/example_2.cpp)
target_link_libraries(example_2 PUBLIC sgrid)


# ##############################################################################
# ##############################################################################
# Installation
# ##############################################################################

install(
    TARGETS sgrid
    EXPORT sgridTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/sgrid_config.hpp.in
    ${CMAKE_BINARY_DIR}/sgrid_config.hpp)

install(FILES ${CMAKE_BINARY_DIR}/sgrid_config.hpp DESTINATION include)
install(FILES ${SGRID_HEADERS} DESTINATION include)

install(
    EXPORT sgridTargets
    FILE sgridTargets.cmake
    NAMESPACE sgrid::
    DESTINATION lib/cmake)

# ##############################################################################

