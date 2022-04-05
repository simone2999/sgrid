cmake_minimum_required(VERSION 3.5...3.19)
cmake_policy(SET CMP0048 NEW)

project(
    main
    VERSION 0.0.1
    LANGUAGES "CXX" "C"
    HOMEPAGE_URL "https://bitbucket.org/zulianp/sgrid/"
    DESCRIPTION
        "Structured grid rid manager"
)

include(CMakePackageConfigHelpers)
include(cmake/SGridDefaults.cmake)
include(cmake/SGridFunctions.cmake)
include(cmake/SGridOptions.cmake)

include(cmake/SGridDependencies.cmake)

# ##############################################################################
# ##############################################################################
# Library
# ##############################################################################

set(SGRID_MODULES core)

foreach(MODULE ${SGRID_MODULES})
    add_subdirectory("${MODULE}")
endforeach(MODULE)

# message(STATUS "HERE:\n
#               SGRID_DEP_INCLUDES=${SGRID_DEP_INCLUDES}\n
#               SGRID_DEP_LIBRARIES=${SGRID_DEP_LIBRARIES}
#               ")

add_library(sgrid ${SGRID_SOURCES} ${SGRID_HEADERS})

foreach(include ${SGRID_BUILD_INCLUDES})
    target_include_directories(sgrid BEFORE
                               PUBLIC $<BUILD_INTERFACE:${include}>)
endforeach()

target_include_directories(sgrid SYSTEM PUBLIC ${SGRID_DEP_INCLUDES})
target_link_libraries(sgrid PUBLIC ${SGRID_DEP_LIBRARIES})
target_link_libraries(sgrid PUBLIC ${SGRID_DEP_TARGETS})
sgrid_set_compiler_features(sgrid)

target_include_directories(
    sgrid BEFORE
    PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
           $<INSTALL_INTERFACE:include>)

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

# generate the config file that is includes the exports
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sgridConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/sgridConfig.cmake"
    INSTALL_DESTINATION "lib/cmake")

# generate the version file for the config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/sgridConfigVersion.cmake"
    VERSION "${MOONOLITH_VERSION}"
    COMPATIBILITY AnyNewerVersion)

# install the configuration file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sgridConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/sgridConfigVersion.cmake
        DESTINATION lib/cmake/)

# ##############################################################################
# ##############################################################################
# Executables.

set(NUMBER 0)

list(
  APPEND
  EXECUTABLES
  drivers/example_1.cpp
  drivers/example_2.cpp
  drivers/example_3.cpp
  drivers/example_4.cpp
  drivers/example_5.cpp
  drivers/example_6.cpp
  drivers/example_7.cpp
  drivers/example_8.cpp
  drivers/example_9.cpp)

foreach(EXECUTABLE ${EXECUTABLES})
  math(EXPR NUMBER "${NUMBER}+1")
  add_executable(sgrid_example_${NUMBER} drivers/example_${NUMBER}.cpp)
  target_link_libraries(sgrid_example_${NUMBER} PUBLIC sgrid)
endforeach(EXECUTABLE)

include(cmake/PostInstall.cmake)