#include <benchmark/benchmark.h>
#include "mpi.h"
#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_SliceHalo.hpp"

// BENCHMARK_MAIN();

int main(int argc, char** argv) {
    using namespace sgrid;

    MPI_Init(&argc, &argv);
    sgrid::initialize(argc, argv);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();

    sgrid::finalize();
    MPI_Finalize();
}