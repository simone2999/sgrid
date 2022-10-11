#include <mpi.h>
#include <cmath>
#include "gtest/gtest.h"

#include "sgrid_Base.hpp"
#include "sgrid_Grid.hpp"

using Real = double;

// TODO: Finish these tests.

TEST(GRIDTest, testCommCoord) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {10, 10}, {0, 0});
    if (grid.comm_size() == 4) {
        if (grid.comm_rank() == 0) {
            ASSERT_TRUE(grid.comm_coord(0) == 0);
            ASSERT_TRUE(grid.comm_coord(1) == 0);
            ASSERT_TRUE(grid.comm_coord(2) == 0);
        } else if (grid.comm_rank() == 1) {
            ASSERT_TRUE(grid.comm_coord(0) == 0);
            ASSERT_TRUE(grid.comm_coord(1) == 1);
            ASSERT_TRUE(grid.comm_coord(2) == 0);
        } else if (grid.comm_rank() == 2) {
            ASSERT_TRUE(grid.comm_coord(0) == 1);
            ASSERT_TRUE(grid.comm_coord(1) == 0);
            ASSERT_TRUE(grid.comm_coord(2) == 0);
        } else if (grid.comm_rank() == 3) {
            ASSERT_TRUE(grid.comm_coord(0) == 1);
            ASSERT_TRUE(grid.comm_coord(1) == 1);
            ASSERT_TRUE(grid.comm_coord(2) == 0);
        }
    }
}

TEST(GRIDTest, testCommRank) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {10, 10}, {0, 0});
    if (grid.comm_size() == 1) {
        ASSERT_TRUE(grid.comm_rank() == 0);
    }
}

// TEST(GRIDTest, testCanShift) {
//     sgrid::Grid<Real, 2> grid;
//     grid.init(MPI_COMM_WORLD, {100, 100}, {1, 1});
//     if (grid.comm_size() == 1) {
//         // Need to figure out what this does.
//         // ASSERT_TRUE(grid.can_shift(0, 200) == true);
//     }
// }

TEST(GRIDTest, testCommDim) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {10, 10}, {1, 1});
    if (grid.comm_size() == 4) {
        // Parameters are x,y,z
        ASSERT_TRUE(grid.comm_dim(0) == 2);
        ASSERT_TRUE(grid.comm_dim(1) == 2);
    }
}
TEST(GRIDTest, testIsPeriodic) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    ASSERT_TRUE(grid.is_periodic(0) == false);
}

TEST(GRIDTest, testShift) {
    sgrid::Grid<Real, 2> grid;
    grid.init(MPI_COMM_WORLD, {10, 10}, {0, 0});
    if (grid.comm_size() == 4) {
        grid.describe();
        grid.shift(0, 1);
        grid.describe();
    }
}

// TEST(GRIDTest, testNeigh) {
//     sgrid::Grid<Real, 2> grid;
//     grid.init(MPI_COMM_WORLD, {100, 100}, {0, 0});
//     // ASSERT_TRUE(grid.neigh(0, 1) == -2);
// }