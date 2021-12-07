#ifndef SGRID_VIEW_HPP
#define SGRID_VIEW_HPP

#ifdef SGRID_WITH_KOKKOS
////////////////////////////////////////////////////////////////////////////

#ifndef PDELAB_CPU_ONLY
#ifdef KOKKOS_ENABLE_CUDA
#define PDELAB_USE_GPU
#endif
#endif // PDELAB_CPU_ONLY

#include "sgrid_Base.hpp"

#include "sgrid_Utils.hpp"

#include <Kokkos_Complex.hpp>
#include <Kokkos_Core.hpp>

#define SGRID_INLINE_FUNCTION KOKKOS_INLINE_FUNCTION
#define SGRID_FUNCTION KOKKOS_FUNCTION
#define SGRID_LAMBDA KOKKOS_LAMBDA

namespace sgrid {

using Kokkos::create_mirror_view;
using Kokkos::deep_copy;
using Kokkos::fence;
using Kokkos::finalize;
using Kokkos::initialize;
using Kokkos::parallel_for;
using Kokkos::parallel_reduce;

using Kokkos::MDRangePolicy;
using Kokkos::RangePolicy;
using Kokkos::Rank;
using Kokkos::View;

using Kokkos::complex;

using complex_double_t = Kokkos::complex<double>;
using complex_float_t = Kokkos::complex<float>;

SGRID_DEFINE_MPI_TYPE(complex_double_t, MPI_DOUBLE_COMPLEX)
SGRID_DEFINE_MPI_TYPE(complex_float_t, MPI_COMPLEX)

/////////////////////////////////////////

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

/////////////////////////////////////////

} // namespace sgrid

////////////////////////////////////////////////////////////////////////////
#else // SGRID_WITH_KOKKOS
////////////////////////////////////////////////////////////////////////////
// Mock protoype. TODO reproduce simplified functionalites of Kokkos
#error "Not supported yet!"

#define SGRID_INLINE_FUNCTION inline
#define SGRID_FUNCTION
#define SGRID_LAMBDA [=]

namespace sgrid {

class HostMemorySpace {};
class DeviceMemorySpace {};

class HostExecutionSpace {};
class DeviceExecutionSpace {};

#ifdef SGRID_WITH_KOKKOS
template <typename T, typename MemorySpace, typename... Args>
using View = ::Kokkos::View<T, MemorySpace, Args...>;
#else

template <typename T, typename MemorySpace, typename... Args> class View {
public:
  using HostMirror = View;

  template <typename... CArgs> View(CArgs &&...) {}

  inline auto &operator[](const int i) { return ptr_[i]; }
  inline auto &operator[](const int i) const { return ptr_[i]; }

  T &data() { return ptr_; }
  const T &data() const { return ptr_; }

private:
  T ptr_;
};

template <typename... Args> class RangePolicy {};
template <typename... Args> class MDRangePolicy {
public:
  template <typename... CArgs> MDRangePolicy(CArgs &&...) {}
  using point_type = int[3];
};
template <int Dim> class Rank {};

template <typename Dest, typename Src> void deep_copy(Dest &&, Src &&) {}

template <class T> inline T create_mirror_view(T &&obj) {
  return std::forward<T>(obj);
}

template <typename... Args> void parallel_for(Args...) {}
template <typename... Args> void parallel_reduce(Args...) {}

void initialize(int, char **) {}
void finalize() {}
void fence() {}

////////////////////////////////////////////////////////////////////////////
#endif
} // namespace sgrid

#endif // SGRID_WITH_KOKKOS

#endif // SGRID_VIEW_HPP