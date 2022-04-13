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
  message(VERBOSE "Found Kokkos")
  set(WITH_KOKKOS ON)
  # check what was found
  message(VERBOSE "Kokkos_CXX_FLAGS: ${Kokkos_CXX_FLAGS}")
  message(VERBOSE "Kokkos_CXX_COMPILER = ${Kokkos_CXX_COMPILER}")
  message(VERBOSE "Kokkos_INCLUDE_DIRS = ${Kokkos_INCLUDE_DIRS}")
  message(VERBOSE "Kokkos_LIBRARIES = ${Kokkos_LIBRARIES}")
  message(VERBOSE "Kokkos_TPL_LIBRARIES = ${Kokkos_TPL_LIBRARIES}")
  message(VERBOSE "Kokkos_LIBRARY_DIRS = ${Kokkos_LIBRARY_DIRS}")

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

  if(SGRID_ENABLE_KOKKOS_CUDA)
    if(NOT DEFINED Kokkos_ENABLE_CUDA OR NOT ${Kokkos_ENABLE_CUDA})
      message(
        FATAL_ERROR
          "Enable Kokkos Cuda or unset SGRID_USE_CUDA to continue with OpenMP!")
    endif()
    message(VERBOSE "Kokkos CUDA Enabled = ${Kokkos_ENABLE_CUDA}")
    # target_compile_definitions(${_KK_TARGET} INTERFACE
    # SGRID_ENABLE_KOKKOS_CUDA)
    add_compile_definitions("SGRID_ENABLE_KOKKOS_CUDA")
    kokkos_check(OPTIONS CUDA_LAMBDA)

    # get cuda flags from the wrapper alternatively we can strip
    # Kokkos_INTERFACE_COMPILE_OPTIONS when defined
    execute_process(
      COMMAND ${Kokkos_CXX_COMPILER} --show
      OUTPUT_VARIABLE _wrapper_command
      ERROR_QUIET)
    string(REGEX REPLACE [[\n\v\c\c]] "" _wrapper_flags ${_wrapper_command})
    string(STRIP "${_wrapper_flags}" _wrapper_flags)
    message(DEBUG "_wrapper_flags ${_wrapper_flags}")

    # this could be done per target if we need to compile other parts of QuICC
    # with different CUDA settings
    set(CMAKE_CUDA_FLAGS "${_wrapper_flags} ${_openmp}")

  else()
    string(FIND "${CMAKE_CXX_FLAGS}" "${_openmp}" _pos)
    if(_pos EQUAL -1)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_openmp}")
    endif()

  endif()

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
  endif()
# if(SGRID_ENABLE_KOKKOS)
#   if(WIN32)
#     find_package(Kokkos HINTS C:/projects/installations/kokkos/lib/cmake/Kokkos
#                  ${Kokkos_DIR} $ENV{KOKKOS_DIR} REQUIRED)
#   else()
#     find_package(Kokkos REQUIRED)
#   endif()
#   message(VERBOSE "Found Kokkos")

#   if(TARGET Kokkos::kokkos)
#     set(SGRID_DEP_TARGETS "${SGRID_DEP_TARGETS};Kokkos::kokkos")

#     # get_target_property(Kokkos_INCLUDE_DIRS Kokkos::kokkos
#     # INTERFACE_INCLUDE_DIRECTORIES) get_target_property(Kokkos_LIBRARIES
#     # Kokkos::kokkos INTERFACE_LINK_LIBRARIES)
#     # get_target_property(Kokkos_LIBRARY_DIRS Kokkos::kokkos
#     # INTERFACE_LINK_DIRECTORIES)

#   else()
#     set(SGRID_DEP_LIBRARIES
#         "${SGRID_DEP_LIBRARIES};${Kokkos_LIBRARIES};${Kokkos_TPL_LIBRARIES}")

#     set(SGRID_DEP_INCLUDES "${SGRID_DEP_INCLUDES};${Kokkos_INCLUDE_DIRS}")
#   endif()

#   # message("\nFound Kokkos!  Here are the details: ") message("
#   # Kokkos_CXX_COMPILER = ${Kokkos_CXX_COMPILER}") message(" Kokkos_INCLUDE_DIRS
#   # = ${Kokkos_INCLUDE_DIRS}") message(" Kokkos_LIBRARIES =
#   # ${Kokkos_LIBRARIES}") message(" Kokkos_TPL_LIBRARIES =
#   # ${Kokkos_TPL_LIBRARIES}") message(" Kokkos_LIBRARY_DIRS =
#   # ${Kokkos_LIBRARY_DIRS}")

#   if(Kokkos_CXX_COMPILER)
#     set(CMAKE_C_COMPILER ${Kokkos_C_COMPILER})
#     set(CMAKE_CXX_COMPILER ${Kokkos_CXX_COMPILER})
#   endif()

#   set(SGRID_WITH_KOKKOS TRUE)
# endif()

# ##############################################################################

if(SGRID_ENABLE_KOKKOS_KERNELS)
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
          "${SGRID_DEP_LIBRARIES};${KokkosKernels_LIBRARIES};${KokkosKernels_TPL_LIBRARIES};-L${KokkosKernels_LIBRARY_DIRS}"
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
  set(SGRID_WITH_KOKKOS TRUE)
endif()

if(CMAKE_BUILD_TYPE MATCHES "[Cc][Oo][Vv][Ee][Rr][Aa][Gg][Ee]")
  include(cmake/CodeCoverage.cmake)
  add_codecov(sgrid_coverage sgrid_test coverage)
endif()
