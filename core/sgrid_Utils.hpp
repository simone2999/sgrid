#ifndef SGRID_UTILS_HPP
#define SGRID_UTILS_HPP

#include "sgrid_Base.hpp"

#include <cassert>
#include <mpi.h>

#define CATCH_MPI_ERROR(err)                                                   \
  {                                                                            \
    if (err != 0) {                                                            \
      assert(false);                                                           \
    }                                                                          \
  }

namespace sgrid {

template <typename A, typename T, typename... Args>
inline void unpack_to_array(A *array, int offset, T first, Args... args) {
  array[offset] = first;
  unpack_to_array(array, offset + 1, std::forward<Args>(args)...);
}

template <typename A, typename T>
inline void unpack_to_array(A *array, int offset, T first) {
  array[offset] = first;
}

template <typename A, typename T1, typename T2>
inline void unpack_to_array(A *array, int offset, T1 first, T2 second) {
  array[offset] = first;
  array[offset + 1] = second;
}

template <typename T> inline MPI_Datatype MPIType() {
  return MPI_DATATYPE_NULL;
}

#define SGRID_DEFINE_MPI_TYPE(_type, _mpi_type)                                \
  template <> inline MPI_Datatype MPIType<_type>() { return _mpi_type; }

SGRID_DEFINE_MPI_TYPE(double, MPI_DOUBLE)
SGRID_DEFINE_MPI_TYPE(float, MPI_FLOAT)

template <typename I>
inline constexpr I tensor_idx_with_margin(const I *dims_with_margin,
                                          const I *margin, const int i) {
  SGRID_UNUSED(dims_with_margin);
  return margin[0] + i;
}

template <typename I>
inline constexpr I tensor_idx_with_margin(const I *dims_with_margin,
                                          const I *margin, const int i,
                                          const int j) {
  return (margin[0] + i) * dims_with_margin[1] + margin[1] + j;
}

template <typename I>
inline constexpr I tensor_idx_with_margin(const I *dims_with_margin,
                                          const I *margin, const int i,
                                          const int j, const int k) {
  return dims_with_margin[2] *
             ((margin[0] + i) * dims_with_margin[1] + margin[1] + j) +
         k;
}

template <typename I>
inline constexpr I tensor_idx(const I *const dims, const int i) {
  SGRID_UNUSED(dims);
  return i;
}

template <typename I>
inline constexpr I tensor_idx(const I *const dims, const int i, const int j) {
  return i * dims[1] + j;
}

template <typename I>
inline constexpr I tensor_idx(const I *const dims, const int i, const int j,
                              const int k) {
  return dims[2] * (i * dims[1] + j) + k;
}

} // namespace sgrid

#endif // SGRID_UTILS_HPP
