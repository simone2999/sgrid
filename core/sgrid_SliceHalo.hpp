#ifndef SGRID_SLICE_HALO_HPP
#define SGRID_SLICE_HALO_HPP

#include "sgrid_SliceNodeHalo.hpp"
#include "sgrid_SliceSideHalo.hpp"

namespace sgrid {

// Exchange edges (3D) or corners of (2d) of a slice
template <class Field, int Dim = Field::Grid::Dim> class SliceHalo {
public:
  using Grid = typename Field::Grid;
  using ValueType = typename Field::ValueType;
  using LocalOrdinal = typename Field::LocalOrdinal;
  using GlobalOrdinal = typename Field::GlobalOrdinal;
  using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
  using HostMirror = typename ViewDevice::HostMirror;

  static_assert(Dim == 3, "Only supports 3D!");

  /// 0=y(z)-plane, 1=x(z)-plane, (2=xy-plane)
  explicit SliceHalo(Field &field, int plane)
      : field_(field), side_(field, plane), node_(field, plane) {}

  void exchange(int slice_number) {
    side_.exchange(slice_number);

    if (field_.stencil_type() == BOX_STENCIL) {
      node_.exchange(slice_number);
    }
  }

private:
  Field &field_;
  SliceSideHalo<Field> side_;
  SliceNodeHalo<Field> node_;
};

} // namespace sgrid

#endif // SGRID_SLICE_HALO_HPP