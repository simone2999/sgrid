#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_View.hpp"

#include "sgrid_SliceHalo.hpp"

#include <mpi.h>

using Grid_t = sgrid::Grid<double, 3>;
using Field_t = sgrid::Field<Grid_t, double>;

// test
int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    Kokkos::initialize(argc, argv);

    {
        int nx = 2;
        int ny = 2;
        int nz = 3;
        int block_size = 1;
        int slice_number = 1;

        if (argc >= 4) {
            nx = atoi(argv[1]);
            ny = atoi(argv[2]);
            nz = atoi(argv[3]);
        }

        if (argc >= 5) {
            block_size = atoi(argv[4]);
        }

        if (argc >= 6) {
            slice_number = atoi(argv[5]);
        }

        bool verbose = false;

        auto grid = std::make_shared<Grid_t>();

        int comm_size;
        MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

        grid->init(MPI_COMM_WORLD, {nx, ny, nz}, {1, 1, 0}, {1, 1, comm_size});

        if (verbose && grid->comm_rank() == 0) {
            printf("Comm grid (%d, %d, %d)\n", grid->comm_dim(0), grid->comm_dim(1), grid->comm_dim(2));
        }

        // create fields
        auto field = std::make_shared<Field_t>("F", grid, block_size, sgrid::BOX_STENCIL);

        field->allocate_on_device();

        auto g_dev = grid->view_device();
        auto x_dev = field->view_device();

        double oracle = grid->comm_rank();

        sgrid::parallel_for(
            "Index", grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                auto b = x_dev.block(i, j, k);
                b[0] = oracle + 1;
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

        static constexpr int slice_exchange_dim = 2;  // We exchange slices in Z

        ///////////////////////////////////////////////////////////////
        // Slice excchange code
        ///////////////////////////////////////////////////////////////
        // Initialize slice halo handler
        sgrid::SideHalo<Field_t> halos(*field);
        halos.init(slice_exchange_dim);
        halos.exchange_slice(slice_number, true);
        ///////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////

        sgrid::parallel_for(
            "Index", grid->md_range_with_ghosts(), SGRID_LAMBDA(int i, int j, int k) {
                ptrdiff_t z = g_dev.global_coord(slice_exchange_dim, k);  // 2=Y
                if (z != slice_number) return;

                // Halos are not communicated by exchange_slice so we skip them
                if (i == 0 || i == g_dev.dim[0] + g_dev.margin[0]) return;
                if (j == 0 || j == g_dev.dim[1] + g_dev.margin[1]) return;

                auto b = x_dev.block(i, j, k);

                if (verbose) {
                    printf("%d, %d, %d ->  %g, %g\n", i, j, k, oracle, b[0]);
                }
            });

        field->write("ex8.raw");

        // sgrid::RawIODebug<Field_t> debug_out(*field);
        // debug_out.set_output_path("ex8_debug.raw");
        // debug_out.write();
    }

    Kokkos::finalize();
    return MPI_Finalize();
}
