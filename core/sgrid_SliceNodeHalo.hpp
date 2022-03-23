#ifndef SGRID_SLICE_NODE_HALO_HPP
#define SGRID_SLICE_NODE_HALO_HPP

namespace sgrid {

    // Exchange edges (3D) or corners of (2d) of a slice
    template <class Field>
    class SliceNodeHalo {
    public:
        using Grid = typename Field::Grid;
        using ValueType = typename Field::ValueType;
        using LocalOrdinal = typename Field::LocalOrdinal;
        using GlobalOrdinal = typename Field::GlobalOrdinal;
        using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
        using HostMirror = typename ViewDevice::HostMirror;
        static constexpr int Dim = Grid::Dim;
        static constexpr int MaxDim = 3;

        static_assert(Dim <= MaxDim, "Only supports 3D and below");

        /// 0=y(z)-plane, 1=x(z)-plane, (2=xy-plane)
        explicit SliceNodeHalo(Field &field, int plane) : field_(field), plane_(plane) { init(); }

        ~SliceNodeHalo() { destroy(); }

        void exchange(int slice_number) {
            auto grid = field_.grid();
            auto g_host = grid->view_host();

            if (slice_number < g_host.start[plane_] || slice_number >= (g_host.start[plane_] + g_host.dim[plane_]))
                return;

            int slice_local_coord = g_host.margin[plane_] + slice_number - g_host.start[plane_];

            ///////////////////////////////////////////////////////////////////

            int disp[MaxDim][2];
            int proc_coord_disp[MaxDim];

            auto field_host = field_.view_host();

            int block_size = field_.block_size();
            MPI_Datatype real_type = MPIType<ValueType>();

            // Generate displacements
            for (int d = 0; d < Grid::Dim; ++d) {
                for (int dir = 0; dir < 2; ++dir) {
                    disp[d][dir] = (grid->comm_coord(d) % 2 == dir) ? -1 : 1;
                }
            }

            for (int d = Grid::Dim; d < MaxDim; ++d) {
                disp[d][0] = 0;
                disp[d][1] = 0;
            }

            disp[plane_][0] = 0;
            disp[plane_][1] = 0;

            int recv_idx[MaxDim];
            int send_idx[MaxDim];
            int tensor_idx[MaxDim];
            int n[MaxDim];

            for (int d = 0; d < Grid::Dim; ++d) {
                n[d] = 2;
            }

            for (int d = Grid::Dim; d < MaxDim; ++d) {
                n[d] = 1;
            }

            n[plane_] = 1;

            for (int i = 0; i < n[0]; ++i) {
                for (int j = 0; j < n[1]; ++j) {
                    for (int k = 0; k < n[2]; ++k) {
                        proc_coord_disp[0] = disp[0][i];
                        proc_coord_disp[1] = disp[1][j];
                        proc_coord_disp[2] = disp[2][k];

                        const int neigh_rank = grid->p_neigh(proc_coord_disp);

                        // assert(neigh_rank != grid->comm_rank());

                        if (neigh_rank == MPI_PROC_NULL) continue;

                        tensor_idx[0] = i;
                        tensor_idx[1] = j;
                        tensor_idx[2] = k;

                        for (int d = 0; d < Grid::Dim; ++d) {
                            if (d == plane_) continue;

                            bool sending_right = (disp[d][tensor_idx[d]] > 0);
                            bool receving_left = sending_right;

                            send_idx[d] = sending_right ? (g_host.dim[d] - 1 + g_host.margin[d]) : g_host.margin[d];
                            recv_idx[d] = receving_left ? 0 : (g_host.dim[d] - 1 + 2 * g_host.margin[d]);
                        }

                        send_idx[plane_] = slice_local_coord;
                        recv_idx[plane_] = slice_local_coord;

                        auto send_ptr = field_host.p_block(send_idx);
                        auto recv_ptr = field_host.p_block(recv_idx);

                        int tag = 0;

                        CATCH_MPI_ERROR(MPI_Sendrecv(send_ptr,
                                                     block_size,
                                                     real_type,
                                                     neigh_rank,
                                                     tag,
                                                     recv_ptr,
                                                     block_size,
                                                     real_type,
                                                     neigh_rank,
                                                     tag,
                                                     grid->raw_comm(),
                                                     MPI_STATUS_IGNORE));
                    }
                }
            }
        }

    private:
        Field &field_;
        int plane_;

        void destroy() {}

        void init() {}
    };
}  // namespace sgrid

#endif  // SGRID_SLICE_NODE_HALO_HPP
