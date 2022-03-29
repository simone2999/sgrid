#ifndef SGRID_SLICE_HALO_HPP
#define SGRID_SLICE_HALO_HPP

#include "sgrid_SliceNodeHalo.hpp"
#include "sgrid_SliceSideHalo.hpp"

namespace sgrid {

    // Exchange edges (3D) or corners of (2d) of a slice
    template <class Field, int Dim = Field::Grid::Dim>
    class SliceHalo {
    public:
        using Grid = typename Field::Grid;
        using ValueType = typename Field::ValueType;
        using LocalOrdinal = typename Field::LocalOrdinal;
        using GlobalOrdinal = typename Field::GlobalOrdinal;
        using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
        using HostMirror = typename ViewDevice::HostMirror;

        explicit SliceHalo(Field &field, int plane) : field_(field), node_(field, plane) {
            // if (SerialSliceSideHalo<Field>::is_valid_handler(field_, plane)) {
            //     if (field_.grid()->comm_rank() == 0) {
            //         std::cout << "Using serial slice halo handler!\n";
            //     }
            //     side_ = std::make_shared<SerialSliceSideHalo<Field>>(field_, plane);
            // } else {
                side_ = std::make_shared<SliceSideHalo<Field>>(field_, plane);
            // }
        }

        /// @param slice_number local index coordinate
        void exchange(int slice_number) {
            side_->exchange(slice_number);

            // if (field_.stencil_type() == BOX_STENCIL) {
            //     node_.exchange(slice_number);
            // }
        }

    private:
        Field &field_;
        // SliceSideHalo<Field> side_;
        std::shared_ptr<SliceSideHaloBase> side_;
        SliceNodeHalo<Field> node_;
    };

}  // namespace sgrid

#endif  // SGRID_SLICE_HALO_HPP