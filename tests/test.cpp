#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_SliceHalo.hpp"
#include "sgrid_field_test.hpp"
#include "sgrid_grid_test.hpp"

#include "gtest/gtest.h"

int main(int argc, char **argv) {
    using namespace sgrid;
    // Need to initialize mpi since sgrid::initialize does not do it.
    MPI_Init(&argc, &argv);
    sgrid::initialize(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    int ok = RUN_ALL_TESTS();

    sgrid::finalize();
    MPI_Finalize();
    return ok;
}