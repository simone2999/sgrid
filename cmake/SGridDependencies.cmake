# MPI
find_package(MPIExtended REQUIRED)

if (MPI_FOUND)
    if(MPI_C_INCLUDE_PATH)
        set(SGRID_DEP_INCLUDES
            "${SGRID_DEP_INCLUDES};${MPI_C_INCLUDE_PATH}")
    endif()

    if(MPI_CXX_INCLUDE_PATH)
        set(SGRID_DEP_INCLUDES
            "${SGRID_DEP_INCLUDES};${MPI_CXX_INCLUDE_PATH}")
    endif()

    if(MPI_LIBRARIES)
        set(SGRID_DEP_LIBRARIES
            "${SGRID_DEP_LIBRARIES};${MPI_LIBRARIES}")
    endif()

    if(MPI_C_LIBRARIES)
        set(SGRID_DEP_LIBRARIES
            "${SGRID_DEP_LIBRARIES};${MPI_C_LIBRARIES}")
    endif()

    if(MPI_CXX_LIBRARIES)
        set(SGRID_DEP_LIBRARIES
            "${SGRID_DEP_LIBRARIES};${MPI_CXX_LIBRARIES}")
    endif()
    # else()
    #     message(FATAL_ERROR "MPI REQUIRED")
endif()



########################################################################################################################

if(SGRID_ENABLE_KOKKOS)
    find_package(Kokkos REQUIRED)

    if (TARGET Kokkos::kokkos)
        set(SGRID_DEP_TARGETS "${SGRID_DEP_TARGETS};Kokkos::kokkos")

        # get_target_property(Kokkos_INCLUDE_DIRS Kokkos::kokkos INTERFACE_INCLUDE_DIRECTORIES)
        # get_target_property(Kokkos_LIBRARIES Kokkos::kokkos INTERFACE_LINK_LIBRARIES)
        # get_target_property(Kokkos_LIBRARY_DIRS Kokkos::kokkos INTERFACE_LINK_DIRECTORIES)

    else()
        set(SGRID_DEP_LIBRARIES
            "${SGRID_DEP_LIBRARIES};${Kokkos_LIBRARIES};${Kokkos_TPL_LIBRARIES}")

        set(SGRID_DEP_INCLUDES
            "${SGRID_DEP_INCLUDES};${Kokkos_INCLUDE_DIRS}")
    endif()

    # message("\nFound Kokkos!  Here are the details: ")
    # message(" Kokkos_CXX_COMPILER = ${Kokkos_CXX_COMPILER}")
    # message(" Kokkos_INCLUDE_DIRS = ${Kokkos_INCLUDE_DIRS}")
    # message(" Kokkos_LIBRARIES = ${Kokkos_LIBRARIES}")
    # message(" Kokkos_TPL_LIBRARIES = ${Kokkos_TPL_LIBRARIES}")
    # message(" Kokkos_LIBRARY_DIRS = ${Kokkos_LIBRARY_DIRS}")

    if(Kokkos_CXX_COMPILER)
        set(CMAKE_C_COMPILER ${Kokkos_C_COMPILER})
        set(CMAKE_CXX_COMPILER ${Kokkos_CXX_COMPILER})
    endif()

    set(SGRID_WITH_KOKKOS TRUE)
endif()

########################################################################################################################

if(SGRID_ENABLE_KOKKOS_KERNELS)
    find_package(KokkosKernels REQUIRED)

    if (TARGET Kokkos::kokkoskernels)
        set(SGRID_DEP_TARGETS "${SGRID_DEP_TARGETS};Kokkos::kokkoskernels")
    else()
        set(SGRID_DEP_INCLUDES
            "${SGRID_DEP_INCLUDES};${KokkosKernels_TPL_INCLUDE_DIRS};${KokkosKernels_INCLUDE_DIRS}")

        if(Kokkos_CXX_COMPILER)
            set(SGRID_DEP_LIBRARIES
                "${SGRID_DEP_LIBRARIES};${KokkosKernels_LIBRARIES};${KokkosKernels_TPL_LIBRARIES}")
        else()
            set(SGRID_DEP_LIBRARIES
                "${SGRID_DEP_LIBRARIES};${KokkosKernels_LIBRARIES};${KokkosKernels_TPL_LIBRARIES};-L${KokkosKernels_LIBRARY_DIRS}")
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


