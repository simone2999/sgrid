#include "sgrid_Field.hpp"
#include <mpi.h>
#include <cmath>
#include "sgrid_Base.hpp"

#include "gtest/gtest.h"

using Real = double;
using Grid_t = sgrid::Grid<Real, 2>;
using Field_t = sgrid::Field<Grid_t>;

// TODO: Finish testing this class.

TEST(FIELDTest, allocate_on_device) {
    auto grid = std::make_shared<Grid_t>();
    grid->init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    Field_t field("test", grid);

    auto test_view_device = grid->view_device();
    field.allocate_on_device();
}

TEST(FIELDTest, block_size) {
    auto grid = std::make_shared<Grid_t>();
    const int block_size = 3;
    grid->init(MPI_COMM_WORLD, {100, 100}, {0, 0});
    Field_t field("test", grid, block_size);

    auto test_view_device = grid->view_device();
}
