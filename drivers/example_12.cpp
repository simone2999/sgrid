#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_Reshape.hpp"
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
        int mpi_size;
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

        const int Nx = 5;
        const int Ny = 4;
        const int Nz = 3 * mpi_size;

        const int tile_size = 2;

        // n_tiles size must be a multiple of mpi_size for this application
        const int n_tiles = 2;
        const int block_size = n_tiles * mpi_size * tile_size;

        auto parallel_grid = std::make_shared<Grid_t>();
        parallel_grid->init(MPI_COMM_WORLD, {Nx, Ny, Nz}, {1, 1, 0});
        // parallel_grid->init(MPI_COMM_WORLD, {Nx, Ny}, {1, 1m 9}, {1, 1, mpi_size}); //1D decomposition

        auto parallel_field = std::make_shared<Field_t>("I", parallel_grid, block_size, sgrid::BOX_STENCIL);
        parallel_field->allocate_on_device();

        auto parallel_field_dev = parallel_field->view_device();

        // Initialize parallel field
        sgrid::parallel_for(
            "Processing on subdomain", parallel_grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                auto b = parallel_field_dev.block(i, j, k);

                for (int l = 0; l < block_size; ++l) {
                    b[l] = l;
                }
            });

        auto serial_grid = std::make_shared<Grid_t>();
        serial_grid->init(MPI_COMM_SELF, {Nx, Ny, Nz});

        auto serial_field = std::make_shared<Field_t>("I", serial_grid, tile_size, sgrid::BOX_STENCIL);
        serial_field->allocate_on_device();

        sgrid::Reshape<Field_t> reshape;
        reshape.init(*parallel_field, *serial_field);

        for (int tile_number = 0; tile_number < n_tiles; ++tile_number) {
            reshape.from_pgrid_to_pblock(*parallel_field, *serial_field, tile_number);

            auto serial_field_dev = serial_field->view_device();

            sgrid::parallel_for(
                "Processing on tile", serial_grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                    auto b = serial_field_dev.block(i, j, k);

                    for (int l = 0; l < tile_size; ++l) {
                        // This in combination with initialization should give us a monotonically increasing
                        // block index starting from 1
                        b[l] += 1;
                    }
                });

            reshape.from_pblock_to_pgrid(*serial_field, *parallel_field, tile_number);
        }

        parallel_field->write("ex12.raw");

        int rank = parallel_grid->comm_rank();
        MPI_Barrier(MPI_COMM_WORLD);
        printf("------------------------\n");
        MPI_Barrier(MPI_COMM_WORLD);

        for (int r = 0; r < mpi_size; ++r) {
            if (r == rank) {
                sgrid::parallel_for(
                    "Processing on subdomain", parallel_grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                        auto b = parallel_field_dev.block(i, j, k);

                        printf("[%d] ", rank);
                        for (int l = 0; l < block_size; ++l) {
                            printf("%g ", b[l]);
                        }

                        printf("\n");
                    });
            }

            fflush(stdout);
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    sgrid::finalize();
    return MPI_Finalize();
}