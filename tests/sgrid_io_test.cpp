#include <mpi.h>

#include <cmath>
#include <filesystem>
#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_IO.hpp"

#include "gtest/gtest.h"

namespace fs = std::filesystem;

using Real = double;

using Grid_t = sgrid::Grid<Real, 3>;
using LongIntField_t = sgrid::Field<Grid_t, long>;

TEST(IOTest, init) {
    int n_x = 10;
    int n_y = 10;
    int n_z = 10;
    int block_size = 3;

    auto grid = std::make_shared<Grid_t>();
    grid->init(MPI_COMM_WORLD, {n_x, n_y, n_z}, {1, 1, 0});

    // Create field from grid.
    LongIntField_t field("TestingField", grid, block_size, sgrid::BOX_STENCIL);
    field.allocate_on_device();

    sgrid::IO io(field, "testingIO", "data.raw");
}

TEST(IOTest, write1) {
    int n_x = 10;
    int n_y = 10;
    int n_z = 10;
    int block_size = 3;

    auto grid = std::make_shared<Grid_t>();
    grid->init(MPI_COMM_WORLD, {n_x, n_y, n_z}, {1, 1, 0});

    // Create field from grid.
    LongIntField_t field("TestingField", grid, block_size, sgrid::BOX_STENCIL);
    field.allocate_on_device();

    sgrid::IO io(field, "testingIO", "data.raw");
    const fs::path p = "testingIO";
    const fs::path p1 = fs::current_path();
    io.write();

    ASSERT_TRUE(fs::exists(fs::absolute(p)));
    if (fs::exists(fs::absolute(p))) {
        fs::remove_all(p);
    }
}

// TEST(IOTest, write2) {
//     int n_x = 10;
//     int n_y = 10;
//     int n_z = 10;
//     int block_size = 3;

//     auto grid = std::make_shared<Grid_t>();
//     grid->init(MPI_COMM_WORLD, {n_x, n_y, n_z}, {1, 1, 0});

//     // Create field from grid.
//     LongIntField_t field("TestingField", grid, block_size, sgrid::BOX_STENCIL);
//     field.allocate_on_device();
//     sgrid::IO io(field, "testingIO", "data.raw");

// }