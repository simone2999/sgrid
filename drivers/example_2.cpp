

#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_View.hpp"

#include <fstream>
#include <mpi.h>

using Real = double;

using Grid_t = sgrid::Grid<Real, 3>;
using Field_t = sgrid::Field<Grid_t>;
using IntField_t = sgrid::Field<Grid_t, int>;
using ComplexField_t = sgrid::Field<Grid_t, sgrid::complex<Real>>;

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
  sgrid::initialize(argc, argv);
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

    Field_t x("x", g, block_size, sgrid::BOX_STENCIL);
    // Field_t x("x", g, block_size, sgrid::STAR_STENCIL);
    x.allocate_on_device(); // Only allocates device
    x.init_halos();

    auto x_dev = x.view_device();

    IntField_t idx("idx", g, 1, sgrid::BOX_STENCIL);
    idx.allocate_on_device();
    idx.init_halos();

    auto idx_dev = idx.view_device();

    ComplexField_t c_field("c_field", g, 1, sgrid::BOX_STENCIL);
    c_field.allocate_on_device();
    c_field.init_halos();

    auto c_field_dev = c_field.view_device();

    int c_size;
    MPI_Type_size(MPI_DOUBLE_COMPLEX, &c_size);

    assert(c_size = sizeof(sgrid::complex_double_t));

    sgrid::complex_double_t val(1. * rank, -1. * rank);

    MPI_Allreduce(MPI_IN_PLACE, &val, 1,
                  sgrid::MPIType<sgrid::complex_double_t>(), MPI_SUM,
                  g->raw_comm());

    if (rank == 0) {
      printf("complex: (%g, %g)", val.real(), val.imag());
    }

    sgrid::parallel_for(
        "VolumeLoop", g->md_range(), SGRID_LAMBDA(int i, int j, int k) {
          const Real x = (g_dev.start[0] + i) * hx;
          const Real y = (g_dev.start[1] + j) * hy;
          const Real z = (g_dev.start[2] + k) * hz;
          auto *block = x_dev.block(i, j, k);

          block[0] = (rank + 1) * (x * x + y * y + z * z);

          idx_dev(i, j, k) = i * ny * nz + j * nz + k;

          c_field_dev(i, j, k) = (x * x + y * y + z * z);

          for (int b = 1; b < block_size; ++b) {
            block[b] = (b) * (x * x + y * y + z * z);
          }
        });

    auto slice = g->md_range_slice(2);

    sgrid::parallel_for(
        "SliceLoop", slice, SGRID_LAMBDA(int i, int j) {
          const Real x = (g_dev.start[0] + i) * hx;
          const Real y = (g_dev.start[1] + j) * hy;
          const Real z = (g_dev.start[2] + 0) * hz;

          auto *block = x_dev.block(i, j, 0);

          block[0] = (rank + 1) * (x * x + y * y + z * z);

          idx_dev(i, j, 0) = i * ny * nz + j * nz + 0;

          c_field_dev(i, j, 0) = (x * x + y * y + z * z);

          for (int b = 1; b < block_size; ++b) {
            block[b] = (b) * (x * x + y * y + z * z);
          }
        });

    MPI_Barrier(g->raw_comm());
    Real end = MPI_Wtime();
    Real user_time = end - start;

    if (rank == 0) {
      printf("Device: \"%s\"\n", typeid(sgrid::DeviceExecutionSpace).name());
      printf("Setup and kernel call:\t%g (seconds)\n", user_time);
    }

    ////////////////////////////////////////////////////////////
    // Halo exchange
    ////////////////////////////////////////////////////////////

    start = MPI_Wtime();

    x.exchange_halos();
    idx.exchange_halos();
    c_field.exchange_halos();

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
      idx.write("idx.raw");

      c_field.write("c_field.raw");

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
  sgrid::finalize();
  return MPI_Finalize();
}
