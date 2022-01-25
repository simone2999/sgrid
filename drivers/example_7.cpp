#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_View.hpp"

#include "sgrid_SliceSideHalo.hpp"

#include <mpi.h>

using Grid_t = sgrid::Grid<double, 3>;
using Field_t = sgrid::Field<Grid_t, double>;

// test
int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  Kokkos::initialize(argc, argv);

  {
    auto grid = std::make_shared<Grid_t>();

    grid->init(MPI_COMM_WORLD, {3, 3, 70}, {1, 1, 0});

    if (grid->comm_rank() == 0) {
      printf("Comm grid (%d, %d, %d)\n", grid->comm_dim(0), grid->comm_dim(1),
             grid->comm_dim(2));
    }

    const int block_size_ = 1584;

    // create fields
    auto field =
        std::make_shared<Field_t>("F", grid, block_size_, sgrid::BOX_STENCIL);

    field->allocate_on_device();

    // Initialize data

    sgrid::SliceSideHalo<Field_t> halos(*field, 2);

    /// slice 1
    halos.exchange(1);
  }

  Kokkos::finalize();
  return MPI_Finalize();
}
