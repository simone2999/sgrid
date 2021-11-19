#ifndef SGRID_BASE_HPP
#define SGRID_BASE_HPP

#include <Kokkos_Core.hpp>
#include <cassert>

#define SGRID_UNUSED(_macro_x) (void)_macro_x

// #define PDELAB_CPU_ONLY

#ifndef PDELAB_CPU_ONLY
#ifdef KOKKOS_ENABLE_CUDA
#define PDELAB_USE_GPU
#endif
#endif // PDELAB_CPU_ONLY

#ifdef KOKKOS_ENABLE_OPENMP
using HostMemorySpace = Kokkos::HostSpace;
using HostExecutionSpace = Kokkos::OpenMP;
#else // Serial
using HostMemorySpace = Kokkos::HostSpace;
using HostExecutionSpace = Kokkos::Serial;
#endif

#ifdef PDELAB_USE_GPU
using DeviceMemorySpace = Kokkos::CudaSpace;
using DeviceExecutionSpace = Kokkos::Cuda;
#else
using DeviceMemorySpace = HostMemorySpace;
using DeviceExecutionSpace = HostExecutionSpace;
#endif

using SerialRangeRank1 = Kokkos::RangePolicy<Kokkos::Serial>;
using SerialRangeRank2 = Kokkos::MDRangePolicy<Kokkos::Rank<2>, Kokkos::Serial>;

using HostRangeRank1 = Kokkos::RangePolicy<HostExecutionSpace>;
using HostRangeRank2 =
    Kokkos::MDRangePolicy<Kokkos::Rank<2>, HostExecutionSpace>;

using DeviceRangeRank1 = Kokkos::RangePolicy<DeviceExecutionSpace>;
using DeviceRangeRank2 =
    Kokkos::MDRangePolicy<Kokkos::Rank<2>, DeviceExecutionSpace>;

#endif // SGRID_BASE_HPP