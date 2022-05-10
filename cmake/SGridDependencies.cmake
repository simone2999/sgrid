# MPI
find_package(MPIExtended REQUIRED)

if(MPI_FOUND)
  if(MPI_C_INCLUDE_PATH)
    set(SGRID_DEP_INCLUDES "${SGRID_DEP_INCLUDES};${MPI_C_INCLUDE_PATH}")
  endif()

  if(MPI_CXX_INCLUDE_PATH)
    set(SGRID_DEP_INCLUDES "${SGRID_DEP_INCLUDES};${MPI_CXX_INCLUDE_PATH}")
  endif()

  if(MPI_LIBRARIES)
    set(SGRID_DEP_LIBRARIES "${SGRID_DEP_LIBRARIES};${MPI_LIBRARIES}")
  endif()

  if(MPI_C_LIBRARIES)
    set(SGRID_DEP_LIBRARIES "${SGRID_DEP_LIBRARIES};${MPI_C_LIBRARIES}")
  endif()

  if(MPI_CXX_LIBRARIES)
    set(SGRID_DEP_LIBRARIES "${SGRID_DEP_LIBRARIES};${MPI_CXX_LIBRARIES}")
  endif()
  # else() message(FATAL_ERROR "MPI REQUIRED")
endif()


# ##############################################################################


if(SGRID_ENABLE_KOKKOS)
  message(STATUS "Setup Kokkos")
  list(APPEND CMAKE_MESSAGE_INDENT "${SGRID_CMAKE_INDENT}")

  if(NOT TRILINOS_DIR)
    message(DEBUG "Setting TRILINOS_DIR to $ENV{TRILINOS_DIR}")
    set(TRILINOS_DIR
        $ENV{TRILINOS_DIR}
        CACHE PATH "Directory where Kokkos is installed")
  endif()

  if(NOT KOKKOS_DIR)
    message(DEBUG "Setting KOKKOS_DIR to $ENV{KOKKOS_DIR}")
    set(KOKKOS_DIR
        $ENV{KOKKOS_DIR}
        CACHE PATH "Directory where Kokkos is installed")
  endif()

  if(WIN32)
    find_package(Kokkos HINTS C:/projects/installations/kokkos/lib/cmake/Kokkos
                 ${Kokkos_DIR} $ENV{KOKKOS_DIR} REQUIRED)
  else()

    find_package(
      Kokkos
      HINTS
      ${KOKKOS_DIR}
      ${KOKKOS_DIR}/lib/CMake/Kokkos
      ${KOKKOS_DIR}/lib64/CMake/Kokkos
      ${TRILINOS_DIR}
      ${TRILINOS_DIR}/lib/cmake/Kokkos
      ${TRILINOS_DIR}/lib64/cmake/Kokkos
      REQUIRED)
  endif()
  message(STATUS "Found Kokkos")
  set(WITH_KOKKOS ON)
  # check what was found
  message(STATUS "Kokkos_CXX_FLAGS: ${Kokkos_CXX_FLAGS}")
  message(STATUS "Kokkos_CXX_COMPILER = ${Kokkos_CXX_COMPILER}")
  message(STATUS "Kokkos_INCLUDE_DIRS = ${Kokkos_INCLUDE_DIRS}")
  message(STATUS "Kokkos_LIBRARIES = ${Kokkos_LIBRARIES}")
  message(STATUS "Kokkos_TPL_LIBRARIES = ${Kokkos_TPL_LIBRARIES}")
  message(STATUS "Kokkos_LIBRARY_DIRS = ${Kokkos_LIBRARY_DIRS}")

  # _KK_TARGET is set as a local variable do not use outside this file
  set(_KK_TARGET "Kokkos::kokkos")

  if(Kokkos_ENABLE_OPENMP)
    set(_openmp "-fopenmp")
    # we need to be sure that all targets link against opemp
    add_link_options(${_openmp})
  endif()

  if(NOT TARGET ${_KK_TARGET})
    message(DEBUG "Kokkos target is not defined")
    add_library(${_KK_TARGET} INTERFACE IMPORTED)
    set_property(
      TARGET ${_KK_TARGET}
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Kokkos_INCLUDE_DIRS}
               ${Kokkos_TPL_INCLUDE_DIRS})
    set_property(
      TARGET ${_KK_TARGET} PROPERTY INTERFACE_LINK_LIBRARIES
                                    ${Kokkos_LIBRARIES} ${Kokkos_TPL_LIBRARIES})
    set_property(TARGET ${_KK_TARGET} PROPERTY INTERFACE_LINK_DIRECTORIES
                                               ${Kokkos_LIBRARY_DIRS})
    set_property(TARGET ${_KK_TARGET} PROPERTY INTERFACE_COMPILE_OPTIONS
                                               ${_openmp})
  else()
    message(DEBUG "Kokkos target is defined")
  endif()

  # Check what the (imported) target does
  get_target_property(Kokkos_INTERFACE_COMPILE_OPTIONS ${_KK_TARGET}
                      INTERFACE_COMPILE_OPTIONS)
  message(
    DEBUG
    "Kokkos_INTERFACE_COMPILE_OPTIONS: ${Kokkos_INTERFACE_COMPILE_OPTIONS}")
  get_target_property(Kokkos_INTERFACE_LINK_LIBRARIES ${_KK_TARGET}
                      INTERFACE_LINK_LIBRARIES)
  message(DEBUG
          "Kokkos_INTERFACE_LINK_LIBRARIES: ${Kokkos_INTERFACE_LINK_LIBRARIES}")
  get_target_property(Kokkos_INTERFACE_INCLUDE_DIRECTORIES ${_KK_TARGET}
                      INTERFACE_INCLUDE_DIRECTORIES)
  message(
    DEBUG
    "Kokkos_INTERFACE_INCLUDE_DIRECTORIES: ${Kokkos_INTERFACE_INCLUDE_DIRECTORIES}"
  )

  # perhaps later we can attach this to the target
  add_compile_definitions("SGRID_ENABLE_KOKKOS")

  get_target_property(Kokkos_INTERFACE_LINK_LIBRARIES ${_KK_TARGET}
                      INTERFACE_LINK_LIBRARIES)
  message(DEBUG
          "Kokkos_INTERFACE_LINK_LIBRARIES: ${Kokkos_INTERFACE_LINK_LIBRARIES}")

  add_library(sgrid::Kokkos INTERFACE IMPORTED)
  set_target_properties(
    sgrid::Kokkos
    PROPERTIES INTERFACE_LINK_LIBRARIES "${Kokkos_INTERFACE_LINK_LIBRARIES}"
               INTERFACE_INCLUDE_DIRECTORIES
               "${Kokkos_INTERFACE_INCLUDE_DIRECTORIES}"
               INTERFACE_COMPILE_OPTIONS "${_openmp}")

  target_compile_definitions(sgrid::Kokkos INTERFACE COMPILE_FOR_KOKKOS)

  set(SGRID_DEP_LIBRARIES "${SGRID_DEP_LIBRARIES};Kokkos::kokkos")

  # done with setting up Kokkos target
  unset(_KK_TARGET)
  set(SGRID_WITH_KOKKOS TRUE)
endif()

# ##############################################################################

if(SGRID_ENABLE_KOKKOS_KERNELS AND NOT WIN32)
  find_package(KokkosKernels QUIET)

  if(TARGET Kokkos::kokkoskernels)
    set(SGRID_DEP_TARGETS "${SGRID_DEP_TARGETS};Kokkos::kokkoskernels")
  else()
    set(SGRID_DEP_INCLUDES
        "${SGRID_DEP_INCLUDES};${KokkosKernels_TPL_INCLUDE_DIRS};${KokkosKernels_INCLUDE_DIRS}"
    )

    if(Kokkos_CXX_COMPILER)
      set(SGRID_DEP_LIBRARIES
          "${SGRID_DEP_LIBRARIES};${KokkosKernels_LIBRARIES};${KokkosKernels_TPL_LIBRARIES}"
      )
    else()
      set(SGRID_DEP_LIBRARIES
          "${SGRID_DEP_LIBRARIES};${KokkosKernels_LIBRARIES};${KokkosKernels_TPL_LIBRARIES};${KokkosKernels_LIBRARY_DIRS}"
      )
    endif()
  endif()

  if(KokkosKernels_C_COMPILER)
    set(CMAKE_C_COMPILER "${KokkosKernels_C_COMPILER}")
  endif()

  if(KokkosKernels_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${KokkosKernels_CXX_COMPILER}")
  endif()

  set(SGRID_WITH_KOKKOS_KERNELS TRUE)
endif()

if(CMAKE_BUILD_TYPE MATCHES "[Cc][Oo][Vv][Ee][Rr][Aa][Gg][Ee]")
  include(cmake/CodeCoverage.cmake)
  add_codecov(sgrid_coverage sgrid_test coverage)
endif()
# message ("SGRID_DEP_LIBRARIES : ${SGRID_DEP_LIBRARIES}")
