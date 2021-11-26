# MPI
find_package(MPI COMPONENTS CXX C REQUIRED)

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

# Kokkos
find_package(KokkosKernels REQUIRED)

if(KokkosKernels_FOUND)
    if (TARGET Kokkos::kokkoskernels)
        set(SGRID_DEP_LIBRARIES "${SGRID_DEP_LIBRARIES};Kokkos::kokkoskernels")
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
endif()

if(KokkosKernels_C_COMPILER)
    set(CMAKE_C_COMPILER "${KokkosKernels_C_COMPILER}")
endif()

if(KokkosKernels_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${KokkosKernels_CXX_COMPILER}")
endif()


