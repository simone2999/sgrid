#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_ReMap.hpp"
#include "sgrid_SliceHalo.hpp"

#include <cmath>
#include <fstream>

#include <mpi.h>

using Real = double;

using Grid_t = sgrid::Grid<Real, 3>;
using Field_t = sgrid::Field<Grid_t>;

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    sgrid::initialize(argc, argv);

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();
    {
        int mpi_size;
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

        bool verbose = false;
        bool save_data = false;

        int Nx = 5;
        int Ny = 4;
        int Nz = 3 * mpi_size;
        int tile_size = 2;
        int n_tiles = 2;

        if (argc >= 2)
            Nx = atoi(argv[1]);
        if (argc >= 3)
            Ny = atoi(argv[2]);
        if (argc >= 4)
            Nz = atoi(argv[3]);
        if (argc >= 5)
            tile_size = atoi(argv[4]);
        if (argc >= 6)
            n_tiles = atoi(argv[5]);

        // block_size must be a multiple of mpi_size for this application
        int block_size = n_tiles * mpi_size * tile_size;
        if (argc >= 7)
            block_size = atoi(argv[6]);

        auto parallel_grid = std::make_shared<Grid_t>();
        parallel_grid->init(MPI_COMM_WORLD, { Nx, Ny, Nz }, { 1, 1, 0 }, {}, false);

        assert(parallel_grid->n_nodes() == Nx * Ny * Nz);

        auto parallel_field = std::make_shared<Field_t>("I", parallel_grid, block_size, sgrid::BOX_STENCIL);
        parallel_field->allocate_on_device();

        auto parallel_field_dev = parallel_field->view_device();
        int rank = parallel_grid->comm_rank();

        // Initialize parallel field
        sgrid::parallel_for(
            "Processing on subdomain", parallel_grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                auto b = parallel_field_dev.block(i, j, k);

                for (int l = 0; l < block_size; ++l) {
                    b[l] = l;
                }
            });

        auto serial_grid = std::make_shared<Grid_t>();
        serial_grid->init(MPI_COMM_SELF, { Nx, Ny, Nz }, {}, {}, false);

        auto serial_field = std::make_shared<Field_t>("I", serial_grid, tile_size, sgrid::BOX_STENCIL);
        serial_field->allocate_on_device();

        assert(serial_grid->view_host().margin[0] == 0);
        assert(parallel_grid->view_host().margin[0] == 0);

        MPI_Barrier(MPI_COMM_WORLD);
        double elapsed = MPI_Wtime();
        double processing_time = 0;

        sgrid::ReMap<Field_t> remap;
        remap.init(*parallel_field, *serial_field);
        bool is_uniform = remap.is_uniform();

        for (int tile_number = 0; tile_number < n_tiles; ++tile_number) {
            remap.from_pgrid_to_pblock(*parallel_field, *serial_field, tile_number);

            double processing_elapsed = MPI_Wtime();

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

            processing_time += MPI_Wtime() - processing_elapsed;
            remap.from_pblock_to_pgrid(*serial_field, *parallel_field, tile_number);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        elapsed = MPI_Wtime() - elapsed;

        if (rank == 0) {
            printf(
                "communication + packing/unpacking + slice processing %g (seconds), processing only %g (seconds) %s\n",
                elapsed,
                processing_time,
                (is_uniform ? "A2A" : "A2AV"));
        }

        if (save_data)
            parallel_field->write("ex12.raw");

        if (verbose) {
            MPI_Barrier(MPI_COMM_WORLD);
            printf("------------------------\n");
            MPI_Barrier(MPI_COMM_WORLD);
        }

        for (int r = 0; r < mpi_size; ++r) {
            if (r == rank) {
                sgrid::parallel_for(
                    "Processing on subdomain", parallel_grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                        auto b = parallel_field_dev.block(i, j, k);

                        if (verbose) {
                            printf("[%d] ", rank);
                        }

                        for (int l = 0; l < block_size; ++l) {
                            if (verbose) {
                                printf("%g ", b[l]);
                            }

                            assert(b[l] == l + 1);
                        }

                        if (verbose) {
                            printf("\n");
                        }
                    });
            }

            if (verbose) {
                fflush(stdout);
                MPI_Barrier(MPI_COMM_WORLD);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime();

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("TTS: %g (seconds)\n", end - start);
    }

    sgrid::finalize();
    return MPI_Finalize();
}