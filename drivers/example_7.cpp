#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_IO.hpp"
#include "sgrid_View.hpp"

#include "sgrid_SliceHalo.hpp"

#include <mpi.h>

using Grid_t = sgrid::Grid<double, 3>;
using Field_t = sgrid::Field<Grid_t, double>;
const std::string folder_name = "example_6";
const std::filesystem::path folder_path = folder_name;

// test
int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    Kokkos::initialize(argc, argv);

    {
        int nx = 2;
        int ny = 2;
        int nz = 1;
        int block_size = 1;

        if (argc >= 4) {
            nx = atoi(argv[1]);
            ny = atoi(argv[2]);
            nz = atoi(argv[3]);
        }

        if (argc >= 5) {
            block_size = atoi(argv[4]);
        }

        bool verbose = false;

        auto grid = std::make_shared<Grid_t>();

        grid->init(MPI_COMM_WORLD, {nx, ny, nz}, {1, 1, 0});

        if (verbose && grid->comm_rank() == 0) {
            printf("Comm grid (%d, %d, %d)\n", grid->comm_dim(0), grid->comm_dim(1), grid->comm_dim(2));
        }

        // create fields
        auto field = std::make_shared<Field_t>("F", grid, block_size, sgrid::BOX_STENCIL);

        field->allocate_on_device();

        auto g_dev = grid->view_device();
        auto x_dev = field->view_device();

        double oracle = -2;

        sgrid::parallel_for(
            "Index", grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                auto b = x_dev.block(i, j, k);
                b[0] = oracle;
            });

        if (verbose) {
            for (int r = 0; r < grid->comm_size(); ++r) {
                int rank = grid->comm_rank();

                if (r == rank) {
                    printf("[%d] %d, %d, %d\n", rank, grid->comm_coord(0), grid->comm_coord(1), grid->comm_coord(2));
                }

                fflush(stdout);

                MPI_Barrier(grid->raw_comm());
            }

            MPI_Barrier(grid->raw_comm());
            fflush(stdout);
            MPI_Barrier(grid->raw_comm());
        }

        // Initialize slice halo handler
        sgrid::SliceHalo<Field_t> halos(*field, 2);

        // Exchange halos of slice 0
        // Index is in global coordinates [0, nx) x [0, ny) x [0, nz)
        int local_slice_number = 0;
        halos.exchange(local_slice_number);

        MPI_Barrier(grid->raw_comm());

        sgrid::parallel_for(
            "Index", grid->md_range_with_ghosts(), SGRID_LAMBDA(int i, int j, int k) {
                // ptrdiff_t z = g_dev.global_coord(2, k);  // 2=Y

                if (k != local_slice_number) return;

                ptrdiff_t x = g_dev.global_coord(0, i);  // 2=Y
                ptrdiff_t y = g_dev.global_coord(1, j);  // 2=Y

                auto b = x_dev.block(i, j, k);

                if (oracle == b[0]) return;

                // else print bugs

                bool inner = true;

                if (x == -1) {
                    printf("LEFT\t");
                    inner = false;
                }

                if (x == nx) {
                    printf("RIGHT\t");
                    inner = false;
                }

                if (y == -1) {
                    printf("BOTTOM\t");
                    inner = false;
                }

                if (y == ny) {
                    printf("TOP\t");
                    inner = false;
                }

                if (inner) {
                    printf("INNER\t");
                }

                printf("%d, %d, %d -> %g == %g\n", i, j, k, b[0], oracle);
            });

        sgrid::RawIODebug<Field_t> debug_out(*field);
        debug_out.set_output_path("example_6/x_debug.raw");
        debug_out.write();

        if (grid->comm_rank() == 0) {
            sgrid::IO io(nx, ny, 0, block_size, folder_name);
            io.write();
        }
    }

    Kokkos::finalize();
    return MPI_Finalize();
}
