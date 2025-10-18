#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_SliceHalo.hpp"

#include <cmath>
#include <fstream>

#include <mpi.h>

using Real = double;

using Grid_t = sgrid::Grid<Real, 3>;
using Field_t = sgrid::Field<Grid_t>;

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    sgrid::initialize(argc, argv);

    {
        bool test = 0;
        bool exchange_halos = 1;

        if (argc > 1) {
            test = atoi(argv[1]);
        }

        if (argc > 2) {
            exchange_halos = atoi(argv[2]);
        }

        int mpi_size;
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

        const int N_ = 6;
        const int block_size = 3;

        auto space_grid_ = std::make_shared<Grid_t>();
        space_grid_->init(MPI_COMM_WORLD, {N_, N_, N_}, {1, 1, 0}, {1, 1, mpi_size});

        auto I_field_ = std::make_shared<Field_t>("I", space_grid_, block_size, sgrid::BOX_STENCIL);
        I_field_->allocate_on_device();

        // fill field
        auto field_dev = I_field_->view_device();
        const auto g_dev = space_grid_->view_device();

        int offset = 1;

        sgrid::parallel_for(
            "INIT I", space_grid_->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                ptrdiff_t x = g_dev.global_coord(0, i);  // 0=X
                ptrdiff_t y = g_dev.global_coord(1, j);  // 1=Y
                ptrdiff_t z = g_dev.global_coord(2, k);  // 2=Z

                auto *block = field_dev.block(i, j, k);
                block[0] = x + offset;
                block[1] = y + offset;
                block[2] = z + offset;
            });

        // halos
        sgrid::SliceHalo<Field_t> xy_slice_halos(*I_field_, 2);
        sgrid::SideHalo<Field_t> halos(*I_field_);
        halos.init(2);

        // Local indexing (includes ghosts)
        const int k_start = 0;
        const int k_end = k_start + 2 * g_dev.margin[2] + g_dev.dim[2];

        if (test) {
            I_field_->exchange_halos();
        } else {
            I_field_->synch_device_to_host();

            if (exchange_halos) halos.exchange();

            // For making sure halows are also available
            I_field_->synch_host_to_device();
            I_field_->synch_device_to_host();

            if (exchange_halos) {
                for (int k = k_start; k < k_end; ++k) {
                    xy_slice_halos.exchange(k);
                }
            }

            I_field_->synch_host_to_device();
        }

        MPI_Barrier(MPI_COMM_WORLD);

        for (int r = 0; r < mpi_size; ++r) {
            if (r == space_grid_->comm_rank()) {
                std::cout << "[" << r << "]\n";

                int zeros = 0;
                sgrid::parallel_reduce(
                    "Print I",
                    space_grid_->md_range_with_ghosts(),
                    SGRID_LAMBDA(int i, int j, int k, int &acc) {
                        ptrdiff_t z = g_dev.global_coord(2, k);  // 2=Z

                        if (z == -1 || z == N_) return;

#ifndef NDEBUG
                        int arr[3] = {i, j, k};
                        assert(g_dev.node_idx(i, j, k) == g_dev.p_node_idx(arr));
#endif  // NDEBUG

                        // if(k == 0 || k == g_dev.dim[2] + g_dev.margin[2]) return;

                        // ptrdiff_t x = g_dev.global_coord(0, i);  // 0=X
                        // ptrdiff_t y = g_dev.global_coord(1, j);  // 1=Y

                        auto *block = field_dev.block(i, j, k);

                        if (block[0] == 0 || block[1] == 0 || block[2] == 0) {
                            acc += 1;
                        }

                        // std::cout << "(" << (x + offset) << ", " << (y + offset) << ", " << (z + offset) << ")"
                        //           << "->";

                        std::cout << "(" << (i) << ", " << (j) << ", " << (k) << ")"
                                  << "->";
                        std::cout << "(" << block[0] << ", " << block[1] << ", " << block[2] << ")" << std::endl;
                    },
                    zeros);

                printf("[%d] zero halos %d\n", r, zeros);
            }

            MPI_Barrier(MPI_COMM_WORLD);
        }

        I_field_->synch_device_to_host();
        I_field_->synch_host_to_device();

        sgrid::RawIODebug<Field_t> debug_out(*I_field_);
        debug_out.set_output_path("ex9_debug.raw");
        debug_out.write();
    }

    sgrid::finalize();
    return MPI_Finalize();
}