#include <benchmark/benchmark.h>
#include <mpi.h>
#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_ReMap.hpp"
#include "sgrid_SliceHalo.hpp"

using Real = double;

using Grid_t = sgrid::Grid<Real, 3>;
using Field_t = sgrid::Field<Grid_t>;

static void bench_from_pgrid_to_pblock(benchmark::State& state) {
    int mpi_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    int Nx = 5;
    int Ny = 4;
    int Nz = 3 * mpi_size;
    int tile_size = 2;
    int n_tiles = 2;

    int block_size = n_tiles * mpi_size * tile_size;
    auto parallel_grid = std::make_shared<Grid_t>();
    parallel_grid->init(MPI_COMM_WORLD, {Nx, Ny, Nz}, {1, 1, 0});
    auto parallel_field = std::make_shared<Field_t>("I", parallel_grid, block_size, sgrid::BOX_STENCIL);
    parallel_field->allocate_on_device();
    auto parallel_field_dev = parallel_field->view_device();
    // int rank = parallel_grid->comm_rank();

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

    sgrid::ReMap<Field_t> remap;
    remap.init(*parallel_field, *serial_field);
    // bool is_uniform = remap.is_uniform()

    for (auto _ : state) {
        for (int tile_number = 0; tile_number < n_tiles; ++tile_number) {
            remap.from_pgrid_to_pblock(*parallel_field, *serial_field, tile_number);
        }
    }
}

BENCHMARK(bench_from_pgrid_to_pblock);

//
static void bench_from_pblock_to_pgrid(benchmark::State& state) {
    int mpi_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    int Nx = 5;
    int Ny = 4;
    int Nz = 3 * mpi_size;
    int tile_size = 2;
    int n_tiles = 2;

    int block_size = n_tiles * mpi_size * tile_size;
    auto parallel_grid = std::make_shared<Grid_t>();
    parallel_grid->init(MPI_COMM_WORLD, {Nx, Ny, Nz}, {1, 1, 0});
    auto parallel_field = std::make_shared<Field_t>("I", parallel_grid, block_size, sgrid::BOX_STENCIL);
    parallel_field->allocate_on_device();
    auto parallel_field_dev = parallel_field->view_device();
    // int rank = parallel_grid->comm_rank();

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

    sgrid::ReMap<Field_t> remap;
    remap.init(*parallel_field, *serial_field);

    for (int tile_number = 0; tile_number < n_tiles; ++tile_number) {
        remap.from_pgrid_to_pblock(*parallel_field, *serial_field, tile_number);

        // double processing_elapsed = MPI_Wtime();

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
        for (auto _ : state) {
            // processing_time += MPI_Wtime() - processing_elapsed;
            remap.from_pblock_to_pgrid(*serial_field, *parallel_field, tile_number);
        }
    }
}

BENCHMARK(bench_from_pblock_to_pgrid);

static void bench_parallel_for(benchmark::State& state) {
    int mpi_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    int Nx = 5;
    int Ny = 4;
    int Nz = 3 * mpi_size;
    int tile_size = 2;
    int n_tiles = 2;

    int block_size = n_tiles * mpi_size * tile_size;
    auto parallel_grid = std::make_shared<Grid_t>();
    parallel_grid->init(MPI_COMM_WORLD, {Nx, Ny, Nz}, {1, 1, 0});
    auto parallel_field = std::make_shared<Field_t>("I", parallel_grid, block_size, sgrid::BOX_STENCIL);
    parallel_field->allocate_on_device();
    auto parallel_field_dev = parallel_field->view_device();
    // int rank = parallel_grid->comm_rank();

    for (auto _ : state) {
        // Initialize parallel field
        sgrid::parallel_for(
            "Processing on subdomain", parallel_grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                auto b = parallel_field_dev.block(i, j, k);

                for (int l = 0; l < block_size; ++l) {
                    b[l] = l;
                }
            });
    }
}

BENCHMARK(bench_parallel_for);

static void bench_allocate_on_device(benchmark::State& state) {
    int mpi_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    int Nx = 5;
    int Ny = 4;
    int Nz = 3 * mpi_size;
    int tile_size = 2;
    int n_tiles = 2;

    int block_size = n_tiles * mpi_size * tile_size;
    auto parallel_grid = std::make_shared<Grid_t>();
    parallel_grid->init(MPI_COMM_WORLD, {Nx, Ny, Nz}, {1, 1, 0});
    auto parallel_field = std::make_shared<Field_t>("I", parallel_grid, block_size, sgrid::BOX_STENCIL);

    for (auto _ : state) {
        parallel_field->allocate_on_device();
    }
}

BENCHMARK(bench_allocate_on_device);