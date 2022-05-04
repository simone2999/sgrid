#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_SliceHalo.hpp"

#include "gtest/gtest.h"

using Real = double;

// TEST(GRIDTest, testCommCoord) {
//     sgrid::Grid<Real, 2> grid;

//     grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
//     ASSERT_TRUE(0 == grid.comm_coord(0));
// }

// TEST(GRIDTest, testCommRank) {
//     sgrid::Grid<Real, 2> grid;
//     // Anyway to test commRank?
//     // if (size == 0) {
//     //     grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
//     //     ASSERT_TRUE(0 == grid.comm_rank());
//     // }
// }

// TEST(GRIDTest, testCanShift) {
//     sgrid::Grid<Real, 2> grid;
//     grid.init(MPI_COMM_WORLD, {100, 100}, {1, 1});
//     if (grid.comm_size() == 1) {
//         // Need to figure out what this does.
//         // ASSERT_TRUE(grid.can_shift(0, 200) == true);
//     }
// }

// TEST(GRIDTest, testDescribe) { sgrid::Grid<Real, 2> grid; }
// TEST(GRIDTest, testCommDim) {
//     sgrid::Grid<Real, 2> grid;
//     grid.init(MPI_COMM_WORLD, {100, 100}, {1, 1});
//     if (grid.comm_rank() == 0) {
//         ASSERT_TRUE(grid.comm_dim(0) == 1);
//         ASSERT_TRUE(grid.comm_dim(1) == 1);
//     }
// }
// TEST(GRIDTest, testIsPeriodic) { sgrid::Grid<Real, 2> grid; }

int main(int argc, char **argv) {
    using namespace sgrid;
    // Need to initialize mpi since sgrid::initialize does not do it.
    MPI_Init(&argc, &argv);
    sgrid::initialize();

    ::testing::InitGoogleTest(&argc, argv);
    int ok = RUN_ALL_TESTS();

    sgrid::finalize();
    MPI_Finalize();
    return ok;
}