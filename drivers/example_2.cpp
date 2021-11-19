

#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"

#include "KokkosBlas1_axpby.hpp"
#include "KokkosBlas1_nrm2.hpp"

#include <fstream>
#include <mpi.h>

using Real = double;

using Grid_t = sgrid::Grid<Real, 3>;
using Field_t = sgrid::Field<Grid_t>;

/**
 * @brief Largest run
 * ICS cluster with 12 slim nodes (20 MPI tasks per node, i.e. 240 processes)
 * Grid: 2000 x 2000 x 2000 x 4 = 32e9
 * File size: 239GB
 * Timings
 * Setup + Echange: 2.21006 (seconds)
 * Output: 789.667 (seconds)
 * We should try to reaqch 4 * 10^12. (Max size heard of 10^14?)
 */
int main(int argc, char *argv[]) {

  MPI_Init(&argc, &argv);
  Kokkos::initialize(argc, argv);
  ////////////////////////////////////////////////////////////////////////

  {
    ////////////////////////////////////////////////////////////
    // Memory allocation
    ////////////////////////////////////////////////////////////

    Real start = MPI_Wtime();

    // Grid parameters
    int nx = 10;
    int ny = 10;
    int nz = 10;
    int block_size = 1;
    bool write_output = true;

    if (argc >= 3) {
      nx = atoi(argv[1]);
      ny = atoi(argv[2]);
      nz = atoi(argv[3]);
    }

    if (argc >= 5) {
      block_size = atoi(argv[4]);
    }

    if (argc >= 6) {
      write_output = atoi(argv[5]);
    }

    // Geometry (unit cube)
    Real hx = 1. / (nx - 1);
    Real hy = 1. / (ny - 1);
    Real hz = 1. / (nz - 1);

    auto g = std::make_shared<Grid_t>();
    g->init(MPI_COMM_WORLD, {nx, ny, nz}, {0, 0, 0});
    // g->describe();
    int rank = g->comm_rank();

    auto g_dev = g->view_device();

    ////////////////////////////////////////////////////////////
    // Memory allocation
    ////////////////////////////////////////////////////////////

    Field_t x("x", g, block_size, sgrid::STAR_STENCIL);
    // Field_t x("x", g, block_size, sgrid::CROSS_STENCIL);
    x.allocate_on_device(); // Only allocates device

    auto x_dev = x.view_device();

    Kokkos::parallel_for(
        "RHS", g->md_range(), KOKKOS_LAMBDA(int i, int j, int k) {
          const Real x = (g_dev.start[0] + i) * hx;
          const Real y = (g_dev.start[1] + j) * hy;
          const Real z = (g_dev.start[2] + k) * hz;
          auto *block = x_dev.block(i, j, k);

          block[0] = (rank + 1) * (x * x + y * y + z * z);

          for (int b = 1; b < block_size; ++b) {
            block[b] = (b) * (x * x + y * y + z * z);
          }
        });

    MPI_Barrier(g->raw_comm());
    Real end = MPI_Wtime();
    Real user_time = end - start;

    if (rank == 0) {
      printf("Device: \"%s\"\n", typeid(DeviceExecutionSpace).name());
      printf("Setup and kernel call:\t%g (seconds)\n", user_time);
    }

    ////////////////////////////////////////////////////////////
    // Halo exchange
    ////////////////////////////////////////////////////////////

    start = MPI_Wtime();

    x.exchange_halos();

    MPI_Barrier(g->raw_comm());

    end = MPI_Wtime();
    user_time = end - start;

    if (rank == 0) {
      printf("Exchange:\t\t%g (seconds)\n", user_time);
    }

    ////////////////////////////////////////////////////////////
    // Output
    ////////////////////////////////////////////////////////////
    if (write_output) {

      start = MPI_Wtime();

      x.write("data.raw");

      MPI_Barrier(g->raw_comm());

      end = MPI_Wtime();
      user_time = end - start;

      if (rank == 0) {
        printf("Output Time:\t\t%g (seconds)\n", user_time);
      }
    }
    ////////////////////////////////////////////////////////////////////////
  }

  ////////////////////////////////////////////////////////////////////////
  Kokkos::finalize();
  return MPI_Finalize();
}
