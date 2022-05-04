#include <mpi.h>
#include <cmath>
#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"

#include "gtest/gtest.h"

TEST(GRIDTest, test1) {
    sgrid::Grid<Real, 2> grid;
    std::cout << grid.comm_rank();
}

TEST(GRIDTest, test2) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "hello");
    // Expect equality.
    EXPECT_EQ(5 * 6, 42);
}
TEST(GRIDTest, test3) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "hello");
    // Expect equality.
    EXPECT_EQ(5 * 6, 42);
}

asdasd
