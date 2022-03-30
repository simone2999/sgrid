#ifndef SGRID_SLICE_SIDE_HALO_HPP
#define SGRID_SLICE_SIDE_HALO_HPP

namespace sgrid {

    class SliceSideHaloBase {
    public:
        virtual ~SliceSideHaloBase() = default;
        virtual void exchange(int slice_number) = 0;
    };

    // template <class Field>
    // class SerialSliceSideHalo : public SliceSideHaloBase {
    // public:
    //     using Grid = typename Field::Grid;
    //     using ValueType = typename Field::ValueType;
    //     using LocalOrdinal = typename Field::LocalOrdinal;
    //     using GlobalOrdinal = typename Field::GlobalOrdinal;
    //     using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
    //     using HostMirror = typename ViewDevice::HostMirror;
    //     static constexpr int Dim = Grid::Dim;

    //     static_assert(Dim == 3, "Only supports 3D");

    //     explicit SerialSliceSideHalo(Field &field, int plane) : field_(field), plane_(plane) {}

    //     static bool is_valid_handler(Field &field, const int plane) {
    //         auto grid = field.grid();

    //         int offset[2] = {0, 0};

    //         switch (plane) {
    //             case 0: {
    //                 offset[0] = 1;
    //                 offset[1] = 2;
    //                 break;
    //             }
    //             case 1: {
    //                 offset[0] = 0;
    //                 offset[1] = 2;
    //                 break;
    //             }
    //             case 2: {
    //                 offset[0] = 0;
    //                 offset[1] = 1;
    //                 break;
    //             }
    //             default:
    //                 break;
    //         }

    //         for (int k = 0; k < 2; ++k) {
    //             if (grid->comm_dim(offset[k]) != 1) {
    //                 // Only works for fully serial planes
    //                 return false;
    //             }
    //         }

    //         return true;
    //     }

    //     void exchange(int slice_number) override {
    //         auto grid = field_.grid();

    //         auto x_dev = field_.view_device();
    //         int block_size = field_.block_size();
    //         auto grid_dev = grid->view_device();

    //         /////////////////////////////////////////

    //         int offset[2] = {0, 0};

    //         switch (plane_) {
    //             case 0: {
    //                 offset[0] = 1;
    //                 offset[1] = 2;
    //                 break;
    //             }
    //             case 1: {
    //                 offset[0] = 0;
    //                 offset[1] = 2;
    //                 break;
    //             }
    //             case 2: {
    //                 offset[0] = 0;
    //                 offset[1] = 1;
    //                 break;
    //             }
    //             default:
    //                 break;
    //         }

    //         for (int k = 0; k < 2; ++k) {
    //             if (grid->comm_dim(offset[k]) != 1) {
    //                 // Only works for fully serial planes
    //                 MPI_Abort(grid->raw_comm(), -1);
    //             }
    //         }

    //         /////////////////////////////////////////

    //         auto g_dev = grid->view_device();

    //         /////////////////////////////////////////

    //         int from[2] = {0, 0};
    //         int to[2] = {0, 0};

    //         for (int k = 0; k < 2; ++k) {
    //             int n = grid_dev.dim[offset[k]];

    //             int start = grid_dev.margin[offset[k]];

    //             // Origin
    //             from[0] = grid_dev.margin[offset[!k]];
    //             to[0] = grid_dev.margin[offset[!k]] + grid_dev.dim[offset[!k]];

    //             // Destination
    //             from[1] = grid_dev.dim[offset[!k]];
    //             to[1] = 0;

    //             for (int l = 0; l < 2; ++l) {
    //                 sgrid::parallel_for(
    //                     "SerialSliceSideHalo", n, SGRID_LAMBDA(int i) {
    //                         int idx_from[3] = {0, 0, 0};
    //                         int idx_to[3] = {0, 0, 0};

    //                         idx_from[plane_] = slice_number;
    //                         idx_to[plane_] = slice_number;

    //                         idx_from[offset[k]] = start + i;
    //                         idx_from[offset[!k]] = from[l];

    //                         idx_to[offset[k]] = start + i;
    //                         idx_to[offset[!k]] = to[l];

    //                         auto *b_from = x_dev.p_block(idx_from);
    //                         auto *b_to = x_dev.p_block(idx_to);

    //                         printf("copy (%d, %d, %d) -> (%d, %d, %d) %g, %g, %g\n",
    //                                idx_from[0],
    //                                idx_from[1],
    //                                idx_from[2],
    //                                idx_to[0],
    //                                idx_to[1],
    //                                idx_to[2],
    //                                b_from[0],
    //                                b_from[1],
    //                                b_from[2]);

    //                         for (int b = 0; b < block_size; ++b) {
    //                             b_to[b] = b_from[b];
    //                         }
    //                     });
    //             }
    //         }
    //     }

    //     Field &field_;
    //     int plane_;
    // };

    // Exchange edges (3D) or corners of (2d) of a slice
    template <class Field, int Dim = Field::Grid::Dim>
    class SliceSideHalo : public SliceSideHaloBase {
    public:
        using Grid = typename Field::Grid;
        using ValueType = typename Field::ValueType;
        using LocalOrdinal = typename Field::LocalOrdinal;
        using GlobalOrdinal = typename Field::GlobalOrdinal;
        using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
        using HostMirror = typename ViewDevice::HostMirror;

        static_assert(Dim == 3, "Only supports 3D");

        /// 0=y(z)-plane, 1=x(z)-plane, (2=xy-plane)
        explicit SliceSideHalo(Field &field, int plane) : field_(field), plane_(plane) { init(); }

        ~SliceSideHalo() { destroy(); }

        void exchange(int slice_local_coord) override {
            auto grid = field_.grid();
            auto grid_host = grid->view_host();

            // if (slice_number < grid_host.start[plane_] ||
            //     slice_number >= (grid_host.start[plane_] + grid_host.dim[plane_]))
            //     return;

            int slice_number = grid_host.start[plane_] + slice_local_coord - grid_host.margin[plane_];
            // int slice_local_coord = grid_host.margin[plane_] + slice_number - grid_host.start[plane_];

            field_.synch_device_to_host();

            auto field_host = field_.view_host();

            int recv_idx[Dim];
            int send_idx[Dim];
            int dirs[2];
            int neigh_rank[2];
            int send_offsets[2];
            int recv_offsets[2];

            for (int dim = 0; dim < Dim; ++dim) {
                // Skip dim of the plane
                if (dim == plane_) continue;

                // Reset array starts
                for (int d = 0; d < Dim; ++d) {
                    recv_idx[d] = grid_host.margin[d];
                    send_idx[d] = grid_host.margin[d];
                }

                // Set plane coordinate to slice number
                recv_idx[plane_] = slice_local_coord;
                send_idx[plane_] = slice_local_coord;

                int proc_coord = grid->comm_coord(dim);
                bool is_even = proc_coord % 2 == 0;

                // Interlace communication
                dirs[0] = is_even ? -1 : 1;
                dirs[1] = is_even ? 1 : -1;

                // Find neighbors
                neigh_rank[0] = grid->shift(dim, dirs[0]);
                neigh_rank[1] = grid->shift(dim, dirs[1]);

                // int line_dim = find_line_dim(dim);

                // Offsets for array
                send_offsets[is_even] = grid_host.margin[dim];
                send_offsets[!is_even] = grid_host.dim[dim];  // includes left margin

                recv_offsets[is_even] = grid_host.dim_with_margin[dim] - 1;
                recv_offsets[!is_even] = 0;

                ////////////////////////////////
                // Send/Recv
                ////////////////////////////////

                for (int k = 0; k < 2; ++k) {
                    if (neigh_rank[k] == MPI_PROC_NULL) continue;
                    int send_tag = k;
                    int recv_tag = k;

                    recv_idx[dim] = recv_offsets[k];
                    send_idx[dim] = send_offsets[k];

                    auto recv_ptr = field_host.p_block(recv_idx);
                    auto send_ptr = field_host.p_block(send_idx);

                    int recv_size, send_size;

                    MPI_Type_size(send_type_[dim], &send_size);
                    MPI_Type_size(recv_type_[dim], &recv_size);

                    printf(
                        "[%d] -> [%d] dim=%d, slice_number=%d, slice_local_coord=%d, (send_size=%d, "
                        "recv_size=%d), phase=%d, "
                        "sp=(%d,%d,%d), "
                        "rp=(%d,%d,%d)\n",
                        grid->comm_rank(),
                        neigh_rank[k],
                        dim,
                        slice_number,
                        slice_local_coord,
                        send_size,
                        recv_size,
                        k,
                        send_idx[0],
                        send_idx[1],
                        send_idx[2],  //
                        recv_idx[0],
                        recv_idx[1],
                        recv_idx[2]);

                    CATCH_MPI_ERROR(MPI_Sendrecv(send_ptr,
                                                 1,
                                                 send_type_[dim],
                                                 neigh_rank[k],
                                                 send_tag,
                                                 recv_ptr,
                                                 1,
                                                 recv_type_[dim],
                                                 neigh_rank[k],
                                                 recv_tag,
                                                 grid->raw_comm(),
                                                 MPI_STATUS_IGNORE));
                }

                field_.synch_host_to_device();
            }

            // void wait_all() {}
        }

    private:
        Field &field_;
        int plane_;

        // One type per side of the slice
        MPI_Datatype recv_type_[Dim];
        MPI_Datatype send_type_[Dim];

        void destroy() {
            for (auto &t : recv_type_) {
                if (t != MPI_DATATYPE_NULL) {
                    MPI_Type_free(&t);
                }
                t = MPI_DATATYPE_NULL;
            }

            for (auto &t : send_type_) {
                if (t != MPI_DATATYPE_NULL) {
                    MPI_Type_free(&t);
                }
                t = MPI_DATATYPE_NULL;
            }
        }

        inline int find_line_dim(const int dim) const {
            int line_dim = -1;

            for (int d = 0; d < Grid::Dim; ++d) {
                if (d != dim && d != plane_) {
                    line_dim = d;
                    break;
                }
            }
            assert(line_dim >= 0);
            return line_dim;
        }

        void init() {
            // TODO check if dimension is serial and periodic

            for (auto &t : recv_type_) {
                t = MPI_DATATYPE_NULL;
            }

            for (auto &t : send_type_) {
                t = MPI_DATATYPE_NULL;
            }

            //
            auto grid = field_.grid();
            auto g_host = grid->view_host();

            MPI_Datatype real_type = MPIType<ValueType>();

            for (int dim = 0; dim < Dim; ++dim) {
                // Skip dim of the plane
                if (dim == plane_) continue;

                int line_dim = find_line_dim(dim);

                int stride = 1;

                for (int d = Grid::Dim - 1; d > line_dim; --d) {
                    stride *= g_host.dim_with_margin[d];
                }

                int count = g_host.dim[line_dim];
                int block_size = field_.block_size();

                if (stride == 1) {
                    CATCH_MPI_ERROR(MPI_Type_contiguous(count * block_size, real_type, &recv_type_[dim]));

                    CATCH_MPI_ERROR(MPI_Type_contiguous(count * block_size, real_type, &send_type_[dim]));

                    // printf("[%d] -> dim=%d, line_dim=%d, count=%d, stride=%d\n",
                    //        grid->comm_rank(), dim, line_dim, count, stride);

                } else {
                    CATCH_MPI_ERROR(
                        MPI_Type_vector(count, block_size, stride * block_size, real_type, &recv_type_[dim]));

                    CATCH_MPI_ERROR(
                        MPI_Type_vector(count, block_size, stride * block_size, real_type, &send_type_[dim]));

                    // printf("[%d] -> dim=%d, line_dim=%d, count=%d, stride=%d\n",
                    //        grid->comm_rank(), dim, line_dim, count, stride);
                }

                MPI_Type_commit(&recv_type_[dim]);
                MPI_Type_commit(&send_type_[dim]);
            }
        }
    };


    template <class Field>
    class SliceSideHalo<Field, 2> : public SliceSideHaloBase {
    public:
        using Grid = typename Field::Grid;
        using ValueType = typename Field::ValueType;
        using LocalOrdinal = typename Field::LocalOrdinal;
        using GlobalOrdinal = typename Field::GlobalOrdinal;
        using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
        using HostMirror = typename ViewDevice::HostMirror;
        static constexpr int Dim = 2;

        static_assert(Dim == 2, "Only supports 2D");

        /// 0=y(z)-plane, 1=x(z)-plane, (2=xy-plane)
        explicit SliceSideHalo(Field &field, int plane) : field_(field), plane_(plane) { init(); }

        ~SliceSideHalo() { destroy(); }

        void init() {}

        void destroy() {}

        void exchange(int slice_local_coord) override {
            (void) slice_local_coord;

        }

        private:
            Field &field_;
            int plane_;

            // One type per side of the slice
            MPI_Datatype recv_type_[Dim];
            MPI_Datatype send_type_[Dim];
    };
}  // namespace sgrid

#endif  // SGRID_SLICE_SIDE_HALO_HPP
