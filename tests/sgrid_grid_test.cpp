#include <mpi.h>
#include <cmath>
#include "gtest/gtest.h"

#include "sgrid_Base.hpp"
#include "sgrid_Grid.hpp"

using Real = double;

TEST(GRIDTest, testCommCoord) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    // ASSERT_TRUE(0 == grid.comm_coord(0));
}

TEST(GRIDTest, testCommRank) {
    sgrid::Grid<Real, 2> grid;
    // Anyway to test commRank?
    // if (size == 0) {
    //     grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    //     ASSERT_TRUE(0 == grid.comm_rank());
    // }
}

TEST(GRIDTest, testCanShift) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {1, 1});
    if (grid.comm_size() == 1) {
        // Need to figure out what this does.
        // ASSERT_TRUE(grid.can_shift(0, 200) == true);
    }
}

TEST(GRIDTest, testDescribe) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    // grid.describe();
}

TEST(GRIDTest, testCommDim) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {1, 1});
    if (grid.comm_size() == 0) {
        // ASSERT_TRUE(grid.comm_dim(0) == 1);
        // ASSERT_TRUE(grid.comm_dim(1) == 1);
    }
}
TEST(GRIDTest, testIsPeriodic) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    ASSERT_TRUE(grid.is_periodic(0) == false);
}
TEST(GRIDTest, testIsProcDims) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    // ASSERT_TRUE(grid.is_proc_dims(0) == false);
}

TEST(GRIDTest, testShift) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    if (grid.comm_size() == 0) {
    }
}

TEST(GRIDTest, testNeigh) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    // ASSERT_TRUE(grid.neigh(0, 1) == -2);
}