#ifndef SGRID_REMAP_HPP
#define SGRID_REMAP_HPP

namespace sgrid {

    template <class Field>
    class ReMap {
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
        void from_pgrid_to_pblock(Field &parallel, Field &serial, int tile_number) {
            parallel.synch_device_to_host();
            p_grid_pack_data(parallel, serial.block_size(), tile_number);

            // int comm_size = parallel.grid()->comm_size();
            // int comm_rank = parallel.grid()->comm_rank();

            // std::cout << std::flush;
            // MPI_Barrier(MPI_COMM_WORLD);
            // for (int r = 0; r < comm_size; ++r) {
            //     if (r == comm_rank) {
            //         int size = p_grid_buffer_.size();
            //         std::cout << "[" << comm_rank << "] tile_number " << tile_number << "/" << serial.block_size()
            //                   << "\n";

            //         for (int i = 0; i < size; ++i) {
            //             std::cout << p_grid_buffer_[i] << " ";
            //         }

            //         std::cout << std::endl;
            //         std::cout << std::flush;
            //     }

            //     MPI_Barrier(MPI_COMM_WORLD);
            // }

            if (profile_) MPI_Barrier(parallel.grid()->raw_comm());
            double elapsed = MPI_Wtime();

            if (is_uniform()) {
                int comm_size = parallel.grid()->comm_size();
                int sendcount = p_grid_buffer_.size() / comm_size;
                int recvcount = p_block_buffer_.size() / comm_size;

                CATCH_MPI_ERROR(MPI_Alltoall(p_grid_buffer_.data(),
                                             sendcount,
                                             MPIType<ValueType>(),
                                             p_block_buffer_.data(),
                                             recvcount,
                                             MPIType<ValueType>(),
                                             parallel.grid()->raw_comm()));
            } else {
                CATCH_MPI_ERROR(MPI_Alltoallv(p_grid_buffer_.data(),
                                              p_grid_a2a_counts_.data(),
                                              p_grid_a2a_displs_.data(),
                                              MPIType<ValueType>(),
                                              p_block_buffer_.data(),
                                              p_block_a2a_counts_.data(),
                                              p_block_a2a_displs_.data(),
                                              MPIType<ValueType>(),
                                              parallel.grid()->raw_comm()));
            }

            if (profile_) MPI_Barrier(parallel.grid()->raw_comm());
            elapsed = MPI_Wtime() - elapsed;
            if (profile_ && parallel.grid()->comm_rank() == 0) {
                printf("Communication (1): %g\n", elapsed);
            }

            serial.ensure_view_host();
            p_block_unpack_data(*parallel.grid(), serial);
            serial.synch_host_to_device();
        }

        void from_pblock_to_pgrid(Field &serial, Field &parallel, int tile_number) {
            serial.synch_device_to_host();
            p_block_pack_data(*parallel.grid(), serial);

            if (profile_) MPI_Barrier(parallel.grid()->raw_comm());
            double elapsed = MPI_Wtime();

            if (is_uniform()) {
                int comm_size = parallel.grid()->comm_size();
                int recvcount = p_grid_buffer_.size() / comm_size;
                int sendcount = p_block_buffer_.size() / comm_size;

                CATCH_MPI_ERROR(MPI_Alltoall(p_block_buffer_.data(),
                                             sendcount,
                                             MPIType<ValueType>(),
                                             p_grid_buffer_.data(),
                                             recvcount,
                                             MPIType<ValueType>(),
                                             parallel.grid()->raw_comm()));
            } else {
                CATCH_MPI_ERROR(MPI_Alltoallv(p_block_buffer_.data(),
                                              p_block_a2a_counts_.data(),
                                              p_block_a2a_displs_.data(),
                                              MPIType<ValueType>(),
                                              p_grid_buffer_.data(),
                                              p_grid_a2a_counts_.data(),
                                              p_grid_a2a_displs_.data(),
                                              MPIType<ValueType>(),
                                              parallel.grid()->raw_comm()));
            }

            if (profile_) MPI_Barrier(parallel.grid()->raw_comm());
            elapsed = MPI_Wtime() - elapsed;
            if (profile_ && parallel.grid()->comm_rank() == 0) {
                printf("Communication (2): %g\n", elapsed);
            }

            parallel.ensure_view_host();
            p_grid_unpack_data(parallel, serial.block_size(), tile_number);
            parallel.synch_host_to_device();

            // int comm_size = parallel.grid()->comm_size();
            // int comm_rank = parallel.grid()->comm_rank();
            // std::cout << std::flush;
            // MPI_Barrier(MPI_COMM_WORLD);

            // for (int r = 0; r < comm_size; ++r) {
            //     if (r == comm_rank) {
            //         std::stringstream ss;
            //         int size = p_grid_buffer_.size();
            //         ss << "[" << comm_rank << "] tile_number " << tile_number << "/"
            //            << (parallel.block_size() / (serial.block_size() * parallel.grid()->comm_size())) << "\n";

            //         for (int i = 0; i < size; ++i) {
            //             ss << p_grid_buffer_[i] << " ";
            //         }

            //         std::cout << ss.str();
            //         std::cout << std::endl;
            //         std::cout << std::flush;
            //     }

            //     MPI_Barrier(MPI_COMM_WORLD);
            // }
        }

        // 1) buffer for data transposition (no MPI subarray types)
        void p_grid_pack_data(Field &parallel_field, int tile_size, int tile_number) {
            auto pgrid = parallel_field.grid();
            auto grid_host = pgrid->view_host();

            auto field_host = parallel_field.view_host();

            auto p_grid_buffer = p_grid_buffer_;

            int comm_size = pgrid->comm_size();
            int block_size = parallel_field.block_size();
            auto rank_offset = pgrid->n_owned_nodes() * tile_size;

            for (int r = 0; r < comm_size; ++r) {
                int block_parition_per_rank = block_size / comm_size;
                int block_offset = (block_parition_per_rank * r) + (tile_number * tile_size);

                if constexpr (Dim == 3) {
                    sgrid::parallel_for(
                        "ReMap::pack_data::CopyBlockSliceToBuffer",
                        pgrid->md_range(),
                        SGRID_LAMBDA(int i, int j, int k) {
                            auto b = field_host.block(i, j, k);

                            int node_offset = (i - grid_host.margin[0]) * (grid_host.dim[1] * grid_host.dim[2]) +
                                              (j - grid_host.margin[1]) * grid_host.dim[2] + (k - grid_host.margin[2]);

                            int data_offset = node_offset * tile_size;

                            for (int l = 0; l < tile_size; ++l) {
                                auto value = b[block_offset + l];
                                p_grid_buffer[r * rank_offset + data_offset + l] = value;
                            }
                        });
                } else if constexpr (Dim == 2) {
                    sgrid::parallel_for(
                        "ReMap::pack_data::CopyBlockSliceToBuffer", pgrid->md_range(), SGRID_LAMBDA(int i, int j) {
                            auto b = field_host.block(i, j);

                            int node_offset =
                                (i - grid_host.margin[0]) * (grid_host.dim[1]) + (j - grid_host.margin[1]);

                            int data_offset = node_offset * tile_size;

                            for (int l = 0; l < tile_size; ++l) {
                                auto value = b[block_offset + l];
                                p_grid_buffer[r * rank_offset + data_offset + l] = value;
                            }
                        });
                }
            }
        }

        void p_grid_unpack_data(Field &parallel_field, int tile_size, int tile_number) {
            auto pgrid = parallel_field.grid();
            auto grid_host = pgrid->view_host();

            auto field_host = parallel_field.view_host();

            auto p_block_buffer = p_block_buffer_;

            int comm_size = pgrid->comm_size();
            int block_size = parallel_field.block_size();
            auto rank_offset = pgrid->n_owned_nodes() * tile_size;

            for (int r = 0; r < comm_size; ++r) {
                int block_parition_per_rank = block_size / comm_size;
                int block_offset = (block_parition_per_rank * r) + (tile_number * tile_size);

                if constexpr (Dim == 3) {
                    sgrid::parallel_for(
                        "ReMap::pack_data::CopyBlockSliceFromBuffer",
                        pgrid->md_range(),
                        SGRID_LAMBDA(int i, int j, int k) {
                            auto b = field_host.block(i, j, k);

                            int node_offset = (i - grid_host.margin[0]) * (grid_host.dim[1] * grid_host.dim[2]) +
                                              (j - grid_host.margin[1]) * grid_host.dim[2] + (k - grid_host.margin[2]);

                            int data_offset = node_offset * tile_size;
                            for (int l = 0; l < tile_size; ++l) {
                                auto value = p_grid_buffer_[r * rank_offset + data_offset + l];
                                b[block_offset + l] = value;
                            }
                        });
                } else if constexpr (Dim == 2) {
                    sgrid::parallel_for(
                        "ReMap::pack_data::CopyBlockSliceFromBuffer", pgrid->md_range(), SGRID_LAMBDA(int i, int j) {
                            auto b = field_host.block(i, j);

                            int node_offset =
                                (i - grid_host.margin[0]) * (grid_host.dim[1]) + (j - grid_host.margin[1]);

                            int data_offset = node_offset * tile_size;
                            for (int l = 0; l < tile_size; ++l) {
                                auto value = p_grid_buffer_[r * rank_offset + data_offset + l];
                                b[block_offset + l] = value;
                            }
                        });
                }
            }
        }

        void p_block_pack_data(Grid &parallel_grid, Field &serial_field) {
            auto grid = serial_field.grid();
            int comm_size = parallel_grid.comm_size();
            int tile_size = serial_field.block_size();

            auto grid_host = grid->view_host();
            auto field_host = serial_field.view_host();

            LocalOrdinal dims[Dim];
            GlobalOrdinal starts[Dim];

            auto p_block_buffer = p_block_buffer_;

            for (int r = 0; r < comm_size; ++r) {
                int proc_offset = p_block_a2a_displs_[r];

                parallel_grid.starts_and_dims(r, starts, dims);

                if constexpr (Dim == 3) {
                    auto range = MDRangeHost({0, 0, 0}, {dims[0], dims[1], dims[2]});

                    sgrid::parallel_for(
                        "ReMap::p_block_pack_data::CopyDataToSlicedBuffer", range, SGRID_LAMBDA(int i, int j, int k) {
                            auto b = field_host.block(starts[0] + i + grid_host.margin[0],
                                                      starts[1] + j + grid_host.margin[1],
                                                      starts[2] + k + grid_host.margin[2]);

                            for (int l = 0; l < tile_size; ++l) {
                                auto value = b[l];
                                p_block_buffer[proc_offset + (i * dims[1] * dims[2] + j * dims[2] + k) * tile_size +
                                               l] = value;
                            }
                        });

                } else if constexpr (Dim == 2) {
                    auto range = MDRangeHost({0, 0}, {dims[0], dims[1]});

                    sgrid::parallel_for(
                        "ReMap::p_block_pack_data::CopyDataToSlicedBuffer", range, SGRID_LAMBDA(int i, int j) {
                            auto b = field_host.block(starts[0] + i + grid_host.margin[0],
                                                      starts[1] + j + grid_host.margin[1]);

                            for (int l = 0; l < tile_size; ++l) {
                                auto value = b[l];
                                p_block_buffer[proc_offset + (i * dims[1] + j) * tile_size + l] = value;
                            }
                        });
                }
            }

            // int comm_rank = parallel_grid.comm_rank();
            // std::cout << std::flush;
            // MPI_Barrier(MPI_COMM_WORLD);
            // for (int r = 0; r < comm_size; ++r) {
            //     if (r == comm_rank) {
            //         int size = p_block_buffer.size();
            //         std::cout << "[" << comm_rank << "]\n";

            //         for (int i = 0; i < size; ++i) {
            //             std::cout << p_block_buffer[i] << " ";
            //         }

            //         std::cout << std::endl;
            //         std::cout << std::flush;
            //     }

            //     MPI_Barrier(MPI_COMM_WORLD);
            // }
        }

        void p_block_unpack_data(Grid &parallel_grid, Field &serial_field) {
            auto grid = serial_field.grid();
            int comm_size = parallel_grid.comm_size();
            int tile_size = serial_field.block_size();

            auto grid_host = grid->view_host();
            auto field_host = serial_field.view_host();

            LocalOrdinal dims[Dim];
            GlobalOrdinal starts[Dim];

            auto p_block_buffer = p_block_buffer_;

            for (int r = 0; r < comm_size; ++r) {
                parallel_grid.starts_and_dims(r, starts, dims);
                int proc_offset = p_block_a2a_displs_[r];

                if constexpr (Dim == 3) {
                    auto r = MDRangeHost({0, 0, 0}, {dims[0], dims[1], dims[2]});

                    sgrid::parallel_for(
                        "ReMap::p_block_pack_data::CopyDataToSlicedBuffer", r, SGRID_LAMBDA(int i, int j, int k) {
                            auto b = field_host.block(starts[0] + i + grid_host.margin[0],
                                                      starts[1] + j + grid_host.margin[1],
                                                      starts[2] + k + grid_host.margin[2]);

                            auto node_offset = proc_offset + (i * dims[1] * dims[2] + j * dims[2] + k) * tile_size;
                            for (int l = 0; l < tile_size; ++l) {
                                auto value = p_block_buffer[node_offset + l];
                                b[l] = value;
                            }
                        });

                } else if constexpr (Dim == 2) {
                    auto r = MDRangeHost({0, 0}, {dims[0], dims[1]});

                    sgrid::parallel_for(
                        "ReMap::p_block_pack_data::CopyDataToSlicedBuffer", r, SGRID_LAMBDA(int i, int j) {
                            auto b = field_host.block(starts[0] + i + grid_host.margin[0],
                                                      starts[1] + j + grid_host.margin[1]);

                            auto node_offset = proc_offset + (i * dims[1] + j) * tile_size;
                            for (int l = 0; l < tile_size; ++l) {
                                auto value = p_block_buffer[node_offset + l];
                                b[l] = value;
                            }
                        });
                }
            }

            // int comm_rank = parallel_grid.comm_rank();
            // std::cout << std::flush;
            // MPI_Barrier(MPI_COMM_WORLD);
            // for (int r = 0; r < comm_size; ++r) {
            //     if (r == comm_rank) {
            //         int size = p_block_buffer.size();
            //         std::cout << "[" << comm_rank << "], " << p_block_a2a_displs_[r] << "\n";

            //         for (int i = 0; i < size; ++i) {
            //             std::cout << p_block_buffer[i] << " ";
            //         }

            //         std::cout << std::endl;
            //         std::cout << std::flush;
            //     }

            //     MPI_Barrier(MPI_COMM_WORLD);
            // }
        }

        void init(Field &parallel_field, Field &serial_field) {
            assert((parallel_field.block_size() / serial_field.block_size()) * serial_field.block_size() ==
                   parallel_field.block_size());

            // Must be serial
            assert(serial_field.grid()->comm_size() == 1);

            auto pgrid = parallel_field.grid();
            auto pblock = serial_field.grid();

            auto n_nodes = pgrid->n_nodes();
            auto n_owned_nodes = pgrid->n_owned_nodes();
            auto tile_size = serial_field.block_size();

            assert(pblock->n_nodes() == n_nodes);
            int comm_size = pgrid->comm_size();

            p_grid_buffer_ = ViewHost("p_grid_buffer", comm_size * n_owned_nodes * tile_size);
            p_block_buffer_ = ViewHost("p_block_buffer", n_nodes * tile_size);

            p_block_a2a_counts_.resize(comm_size);
            p_block_a2a_displs_.resize(comm_size + 1);
            p_block_a2a_displs_[0] = 0;

            p_grid_a2a_counts_.resize(comm_size);
            p_grid_a2a_displs_.resize(comm_size + 1);
            p_grid_a2a_displs_[0] = 0;

            for (auto &c : p_grid_a2a_counts_) {
                c = n_owned_nodes * tile_size;
            }

            for (int r = 0; r < comm_size; ++r) {
                p_grid_a2a_displs_[r + 1] = p_grid_a2a_displs_[r] + p_grid_a2a_counts_[r];
            }

            int dims[Dim];
            for (int r = 0; r < comm_size; ++r) {
                pgrid->dims(r, dims);

                int count = 1;

                for (int d = 0; d < Dim; ++d) {
                    count *= dims[d];
                }

                count *= tile_size;

                p_block_a2a_counts_[r] = count;
                p_block_a2a_displs_[r + 1] = p_block_a2a_displs_[r] + count;
            }

            assert(p_block_a2a_displs_[comm_size] == n_nodes * tile_size);

            int check_count = p_block_a2a_counts_[0];

            is_uniform_ = true;
            for (int r = 1; r < comm_size; ++r) {
                if (check_count != p_block_a2a_counts_[r]) {
                    is_uniform_ = false;
                    break;
                }
            }

            // print_a2a();
            // is_uniform_ = false;
        }

        inline bool is_uniform() const { return is_uniform_; }

        void print_a2a() {
            int rank, size;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            MPI_Barrier(MPI_COMM_WORLD);

            for (int r = 0; r < size; ++r) {
                if (r == rank) {
                    std::cout << "=================================\n";
                    std::cout << "[" << rank << "]\n";
                    std::cout << "is_uniform = " << is_uniform_ << "\n";
                    std::cout << "p_grid\n";

                    for (auto c : p_grid_a2a_counts_) {
                        std::cout << c << " ";
                    }
                    std::cout << "\n";

                    for (auto c : p_grid_a2a_displs_) {
                        std::cout << c << " ";
                    }
                    std::cout << "\n";
                    std::cout << "----------------------------------\n";
                    std::cout << "p_block\n";

                    for (auto c : p_block_a2a_counts_) {
                        std::cout << c << " ";
                    }
                    std::cout << "\n";

                    for (auto c : p_block_a2a_displs_) {
                        std::cout << c << " ";
                    }

                    std::cout << "\n";
                    std::cout << "=================================\n";
                    std::cout << std::flush;
                }

                MPI_Barrier(MPI_COMM_WORLD);
            }
        }

    private:
        ViewHost p_grid_buffer_;
        ViewHost p_block_buffer_;

        bool is_uniform_{false};

        std::vector<int> p_grid_a2a_counts_;
        std::vector<int> p_grid_a2a_displs_;

        std::vector<int> p_block_a2a_counts_;
        std::vector<int> p_block_a2a_displs_;

        bool profile_{true};
    };

}  // namespace sgrid

#endif  // SGRID_REMAP_HPP