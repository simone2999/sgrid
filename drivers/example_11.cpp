#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_Reshape.hpp"
#include "sgrid_SliceHalo.hpp"

#include <cmath>
#include <fstream>

#include <mpi.h>

using Real = double;

using Grid_t = sgrid::Grid<Real, 2>;
using Field_t = sgrid::Field<Grid_t>;

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    sgrid::initialize(argc, argv);

    {
        int mpi_size;
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

        const int Nx = 5;
        const int Ny = 3 * mpi_size;

        const int tile_size = 2;

        // n_tiles size must be a multiple of mpi_size for this application
        const int n_tiles = 2;
        const int block_size = n_tiles * mpi_size * tile_size;

        auto parallel_grid = std::make_shared<Grid_t>();
        parallel_grid->init(MPI_COMM_WORLD, {Nx, Ny}, {1, 0}, {1, mpi_size});

        auto parallel_field = std::make_shared<Field_t>("I", parallel_grid, block_size, sgrid::BOX_STENCIL);
        parallel_field->allocate_on_device();

        auto parallel_field_dev = parallel_field->view_device();

        int rank = parallel_grid->comm_rank();

        // Initialize parallel field
        sgrid::parallel_for(
            "Processing on subdomain", parallel_grid->md_range(), SGRID_LAMBDA(int i, int j) {
                auto b = parallel_field_dev.block(i, j);

                for (int k = 0; k < block_size; ++k) {
                    b[k] = k;
                }
            });

        auto serial_grid = std::make_shared<Grid_t>();
        serial_grid->init(MPI_COMM_SELF, {Nx, Ny});

        auto serial_field = std::make_shared<Field_t>("I", serial_grid, tile_size, sgrid::BOX_STENCIL);
        serial_field->allocate_on_device();

        sgrid::Reshape<Field_t> reshape;
        reshape.init(*parallel_field, *serial_field);

        for (int tile_number = 0; tile_number < n_tiles; ++tile_number) {
            reshape.from_pgrid_to_pblock(*parallel_field, *serial_field, tile_number);

            auto serial_field_dev = serial_field->view_device();

            std::cout << std::flush;
            MPI_Barrier(MPI_COMM_WORLD);

            std::cout << "------------------------\n";
            std::cout << std::flush;
            MPI_Barrier(MPI_COMM_WORLD);

            for (int r = 0; r < mpi_size; ++r) {
                if (r == rank) {
                    printf("[%d]\n", r);
                    sgrid::parallel_for(
                        "Processing on tile", serial_grid->md_range(), SGRID_LAMBDA(int i, int j) {
                            auto b = serial_field_dev.block(i, j);

                            for (int j = 0; j < tile_size; ++j) {
                                // This in combination with initialization should give us a monotonically increasing
                                // block index
                                printf("%g ", b[j]);
                                b[j] += 1;
                            }

                            printf("\n");
                        });

                    std::cout << std::flush;
                }

                MPI_Barrier(MPI_COMM_WORLD);
            }

            reshape.from_pblock_to_pgrid(*serial_field, *parallel_field, tile_number);
        }

        parallel_field->write("ex11.raw");

        std::cout << std::flush;
        MPI_Barrier(MPI_COMM_WORLD);

        std::cout << "------------------------\n";
        std::cout << std::flush;
        MPI_Barrier(MPI_COMM_WORLD);

        for (int r = 0; r < mpi_size; ++r) {
            if (r == rank) {
                sgrid::parallel_for(
                    "Processing on subdomain", parallel_grid->md_range(), SGRID_LAMBDA(int i, int j) {
                        auto b = parallel_field_dev.block(i, j);

                        std::cout << "[" << rank << "] ";
                        for (int k = 0; k < block_size; ++k) {
                            std::cout << b[k] << " ";
                        }

                        std::cout << "\n";
                    });

                std::cout << std::flush;
            }

            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    sgrid::finalize();
    return MPI_Finalize();
}