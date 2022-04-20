#ifndef SGRID_RESHAPE_HPP
#define SGRID_RESHAPE_HPP

namespace sgrid {

    template <class Field>
    class Reshape {
    public:
        using Grid = typename Field::Grid;
        static constexpr int Dim = Grid::Dim;
        using ValueType = typename Field::ValueType;
        using LocalOrdinal = typename Field::LocalOrdinal;
        using GlobalOrdinal = typename Field::GlobalOrdinal;
        using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
        using ViewHost = sgrid::View<ValueType *, HostMemorySpace>;
        using HostMirror = typename ViewDevice::HostMirror;
        using MDRangeHost = typename Grid::MDRangeHost;

        /**
         * @brief This method takes a field distributed with respect to spatial coordinates
         * and redistributes its values spliting the block instead.
         * For instance:
         * Grid g with size: NX x NY x NZ x block_size
         * For the 'in' field NX x NY x NZ are decomposed while block_size is store contiguosly and completely on each
         * process
         * For the 'out' field each process has the complete NX x NY x NZ grid while block_size is split among
         * processes.
         * 1) The sending process will extract the slices in a buffer
         * 2) Data is echanged by means of alltoall communication and stored in a temporary buffer
         * 3) Data is transposed to destination array
         */
        void from_pgrid_to_pblock(Field &in, Field &out, int tile_number) {
            in.synch_device_to_host();
            p_grid_pack_data(in, out.block_size(), tile_number);

            if (is_uniform()) {
                CATCH_MPI_ERROR(MPI_Alltoall(p_grid_buffer_.data(),
                                             p_grid_buffer_.size(),
                                             MPIType<ValueType>(),
                                             p_block_buffer_.data(),
                                             p_block_buffer_.size(),
                                             MPIType<ValueType>(),
                                             in.grid()->raw_comm()));
            } else {
                CATCH_MPI_ERROR(MPI_Alltoallv(p_grid_buffer_.data(),
                                              a2a_counts_.data(),
                                              a2a_displs_.data(),
                                              MPIType<ValueType>(),
                                              p_block_buffer_.data(),
                                              a2a_counts_.data(),
                                              a2a_displs_.data(),
                                              MPIType<ValueType>(),
                                              in.grid()->raw_comm()));
            }

            out.ensure_view_host();
            p_block_unpack_data(out);
            out.synch_host_to_device();
        }

        void from_pblock_to_pgrid(Field &in, Field &out, int tile_number) {
            in.synch_device_to_host();
            p_block_pack_data(in);

            out.ensure_view_host();
            p_grid_pack_data(out, in.block_size(), tile_number);
            out.synch_host_to_device();
        }

        // 1) buffer for data transposition (no MPI subarray types)
        void p_grid_pack_data(Field &pgrid_field, int tile_size, int tile_number) {
            auto pgrid = pgrid_field.grid();
            auto grid_host = pgrid->view_host();

            auto field_host = pgrid_field.view_host();

            auto p_grid_buffer = p_grid_buffer_;

            if constexpr (Dim == 3) {
                sgrid::parallel_for(
                    "Reshape::pack_data::CopyBlockSliceToBuffer", pgrid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                        auto b = field_host.block(i, j, k);

                        int node_offset = (i - grid_host.margin[0]) * (grid_host.dim[1] * grid_host.dim[2]) +
                                          (j - grid_host.margin[1]) * grid_host.dim[2] + (k - grid_host.margin[2]);

                        int data_offset = node_offset * tile_size;

                        for (int l = tile_number * tile_size; l < ((tile_number + 1) * tile_size); ++l) {
                            auto value = b[l];
                            p_grid_buffer[data_offset + l] = value;
                        }
                    });
            } else if constexpr (Dim == 2) {
                sgrid::parallel_for(
                    "Reshape::pack_data::CopyBlockSliceToBuffer", pgrid->md_range(), SGRID_LAMBDA(int i, int j) {
                        auto b = field_host.block(i, j);

                        int node_offset = (i - grid_host.margin[0]) * (grid_host.dim[1]) + (j - grid_host.margin[1]);

                        int data_offset = node_offset * tile_size;

                        for (int l = tile_number * tile_size; l < ((tile_number + 1) * tile_size); ++l) {
                            auto value = b[l];
                            p_grid_buffer[data_offset + l] = value;
                        }
                    });
            }
        }

        void p_grid_unpack_data(Field &pgrid_field, int tile_size, int tile_number) {
            auto pgrid = pgrid_field.grid();
            auto grid_host = pgrid->view_host();

            auto field_host = pgrid_field.view_host();

            auto p_block_buffer = p_block_buffer_;

            if constexpr (Dim == 3) {
                sgrid::parallel_for(
                    "Reshape::pack_data::CopyBlockSliceFromBuffer",
                    pgrid.md_range(),
                    SGRID_LAMBDA(int i, int j, int k) {
                        auto b = field_host.block(i, j, k);

                        int node_offset = (i - grid_host.margin[0]) * (grid_host.dims[1] * grid_host.dims[2]) +
                                          (j - grid_host.margin[1]) * grid_host.dims[2] + (k - grid_host.margin[2]);

                        int data_offset = node_offset * tile_size;

                        for (int l = tile_number * tile_size; l < ((tile_number + 1) * tile_size); ++l) {
                            auto value = p_block_buffer[data_offset + l];
                            b[l] = value;
                        }
                    });
            } else if constexpr (Dim == 2) {
                sgrid::parallel_for(
                    "Reshape::pack_data::CopyBlockSliceFromBuffer", pgrid.md_range(), SGRID_LAMBDA(int i, int j) {
                        auto b = field_host.block(i, j);

                        int node_offset = (i - grid_host.margin[0]) * (grid_host.dims[1]) + (j - grid_host.margin[1]);

                        int data_offset = node_offset * tile_size;

                        for (int l = tile_number * tile_size; l < ((tile_number + 1) * tile_size); ++l) {
                            auto value = p_block_buffer[data_offset + l];
                            b[l] = value;
                        }
                    });
            }
        }

        void p_block_pack_data(Field &pblock_field) {
            auto grid = pblock_field.grid();
            int comm_size = grid->comm_size();
            int tile_size = pblock_field.block_size();

            auto grid_host = grid->view_host();
            auto field_host = pblock_field.view_host();

            LocalOrdinal dims[Dim];
            GlobalOrdinal starts[Dim];

            auto p_block_buffer = p_block_buffer_;

            for (int r = 0; r < comm_size; ++r) {
                grid->starts_and_dims(r, starts, dims);

                if constexpr (Dim == 3) {
                    auto r = MDRangeHost({0, 0}, {dims[0], dims[1], dims[2]});

                    sgrid::parallel_for(
                        "Reshape::p_block_pack_data::CopyDataToSlicedBuffer", r, SGRID_LAMBDA(int i, int j, int k) {
                            auto b = field_host.block(starts[0] + i + grid_host.margin[0],
                                                      starts[1] + j + grid_host.margin[1],
                                                      starts[2] + k + grid_host.margin[2]);

                            for (int l = 0; l < tile_size; ++l) {
                                auto value = b[l];
                                p_block_buffer[i * dims[1] * dims[2] + j * dims[2] + k + l] = value;
                            }
                        });

                } else if constexpr (Dim == 2) {
                    auto r = MDRangeHost({0, 0}, {dims[0], dims[1]});

                    sgrid::parallel_for(
                        "Reshape::p_block_pack_data::CopyDataToSlicedBuffer", r, SGRID_LAMBDA(int i, int j) {
                            auto b = field_host.block(starts[0] + i + grid_host.margin[0],
                                                      starts[1] + j + grid_host.margin[1]);

                            for (int l = 0; l < tile_size; ++l) {
                                auto value = b[l];
                                p_block_buffer[i * dims[1] + j + l] = value;
                            }
                        });
                }
            }
        }

        void p_block_unpack_data(Field &pblock_field) {
            auto grid = pblock_field.grid();
            int comm_size = grid->comm_size();
            int tile_size = pblock_field.block_size();

            auto grid_host = grid->view_host();
            auto field_host = pblock_field.view_host();

            LocalOrdinal dims[Dim];
            GlobalOrdinal starts[Dim];

            auto p_block_buffer = p_block_buffer_;

            for (int r = 0; r < comm_size; ++r) {
                grid->starts_and_dims(r, starts, dims);

                if constexpr (Dim == 3) {
                    auto r = MDRangeHost({0, 0}, {dims[0], dims[1], dims[2]});

                    sgrid::parallel_for(
                        "Reshape::p_block_pack_data::CopyDataToSlicedBuffer", r, SGRID_LAMBDA(int i, int j, int k) {
                            auto b = field_host.block(starts[0] + i + grid_host.margin[0],
                                                      starts[1] + j + grid_host.margin[1],
                                                      starts[2] + k + grid_host.margin[2]);

                            for (int l = 0; l < tile_size; ++l) {
                                auto value = p_block_buffer[i * dims[1] * dims[2] + j * dims[2] + k + l];
                                b[l] = value;
                            }
                        });

                } else if constexpr (Dim == 2) {
                    auto r = MDRangeHost({0, 0}, {dims[0], dims[1]});

                    sgrid::parallel_for(
                        "Reshape::p_block_pack_data::CopyDataToSlicedBuffer", r, SGRID_LAMBDA(int i, int j) {
                            auto b = field_host.block(starts[0] + i + grid_host.margin[0],
                                                      starts[1] + j + grid_host.margin[1]);

                            for (int l = 0; l < tile_size; ++l) {
                                auto value = p_block_buffer[i * dims[1] + j + l];
                                b[l] = value;
                            }
                        });
                }
            }
        }

        void init(Field &pgrid_field, Field &pblock_field) {
            assert((pgrid_field.block_size() / pblock_field.block_size()) * pblock_field.block_size() ==
                   pgrid_field.block_size());

            // Must be serial
            assert(pblock_field.grid()->comm_size() == 1);

            auto pgrid = pgrid_field.grid();
            auto pblock = pblock_field.grid();

            auto n_nodes = pgrid->n_nodes();
            auto n_owned_nodes = pgrid->n_owned_nodes();
            auto tile_size = pblock_field.block_size();

            assert(pblock->n_nodes() == n_nodes);

            p_grid_buffer_ = ViewHost("p_grid_buffer", n_owned_nodes * tile_size);
            p_block_buffer_ = ViewHost("p_block_buffer", n_nodes * tile_size);

            int comm_size = pgrid->comm_size();
            a2a_counts_.resize(comm_size);
            a2a_displs_.resize(comm_size + 1);
            a2a_displs_[0] = 0;

            int dims[Dim];
            for (int r = 0; r < comm_size; ++r) {
                pgrid->dims(r, dims);

                int count = 1;

                for (int d = 0; d < Dim; ++d) {
                    count *= dims[d];
                }

                count *= tile_size;
                a2a_counts_[r] = count;
                a2a_displs_[r + 1] = a2a_displs_[r] + count;
            }

            int check_count = a2a_counts_[0];

            is_uniform_ = true;
            for (int r = 1; r < comm_size; ++r) {
                if (check_count != a2a_counts_[r]) {
                    is_uniform_ = false;
                    break;
                }
            }
        }

        inline bool is_uniform() const { return is_uniform_; }

    private:
        ViewHost p_grid_buffer_;
        ViewHost p_block_buffer_;

        bool is_uniform_{false};

        std::vector<int> a2a_counts_;
        std::vector<int> a2a_displs_;
    };

}  // namespace sgrid

#endif  // SGRID_RESHAPE_HPP