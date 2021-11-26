
#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"

#include <cmath>
#include <fstream>

#include <mpi.h>

using Real = double;

using Grid_t = sgrid::Grid<Real, 2>;
using Field_t = sgrid::Field<Grid_t>;

// Analytic function
SGRID_INLINE_FUNCTION Real fun(const Real x, const Real y) {
  return (x * (1 - x) * y * (1 - y)) * (x + y);
}

// Analytic Laplacian
SGRID_INLINE_FUNCTION Real lapl_fun(const Real x, const Real y) {
  return 2 * (y - 1) * y * (3 * x + y - 1) + 2 * (x - 1) * x * (3 * y + x - 1);
}

int main(int argc, char *argv[]) {

  MPI_Init(&argc, &argv);
  sgrid::initialize(argc, argv);

  ////////////////////////////////////////////////////////

  {
    Real start = MPI_Wtime();

    // Grid parameters
    int nx = 100;
    int ny = 100;

    if (argc == 3) {
      nx = atoi(argv[1]);
      ny = atoi(argv[2]);
    }

    // Geometry (unit square)
    Real hx = 1. / (nx - 1);
    Real hy = 1. / (ny - 1);

    // Solver parameters
    int max_iter = 20 * nx * ny;
    Real atol = 1e-8;
    int check_residual_each = 500;

    auto g = std::make_shared<Grid_t>();
    g->init(MPI_COMM_WORLD, {nx, ny}, {0, 0});
    // g->describe();

    auto g_dev = g->view_device();

    ////////////////////////////////////////////////////////////
    // Memory allocation
    ////////////////////////////////////////////////////////////

    Field_t solution("solution", g);
    solution.allocate_on_device(); // Only allocates device

    Field_t rhs("rhs", g);
    rhs.allocate_on_device(); // Only allocates device

    auto solution_dev = solution.view_device();
    auto rhs_dev = rhs.view_device();

    int rank = g->comm_rank();

    sgrid::parallel_for(
        "Index", g->md_range(), SGRID_LAMBDA(int i, int j) {
          // solution_dev.block(i, j)[0]
          solution_dev.ref(i, j) =
              (g_dev.start[0] + i - g_dev.margin[0]) * g_dev.global_dim[1] +
              (g_dev.start[1] + j - g_dev.margin[1]);
        });

    sgrid::fence();

    ////////////////////////////////////////////////////////////
    // Boundary conditions
    ////////////////////////////////////////////////////////////

    // FIXME (sould only iterate over real boundary)
    sgrid::parallel_for(
        "BC", g->md_range_with_ghosts(), SGRID_LAMBDA(int i, int j) {
          auto x_id = (g_dev.start[0] + i);
          auto y_id = (g_dev.start[1] + j);

          const Real x = x_id * hx;
          const Real y = y_id * hy;

          bool is_boundary =
              x_id == 0 || x_id == (nx + 2) || y_id == 0 || y_id == (ny + 2);

          solution_dev.ref(i, j) = is_boundary ? fun(x, y) : 0.;
        });

    sgrid::fence();

    ////////////////////////////////////////////////////////////
    // RHS
    ////////////////////////////////////////////////////////////

    sgrid::parallel_for(
        "RHS", g->md_range(), SGRID_LAMBDA(int i, int j) {
          const Real x = (g_dev.start[0] + i) * hx;
          const Real y = (g_dev.start[1] + j) * hy;
          rhs_dev.ref(i, j) = lapl_fun(x, y);
        });

    sgrid::fence();

    rhs.exchange_halos();
    rhs.synch_device_to_host();

    ////////////////////////////////////////////////////////////
    // Solve
    ////////////////////////////////////////////////////////////

    for (int iter = 0; iter < max_iter; ++iter) {

      for (int color = 0; color < 2; ++color) {

        // Implementation of Red-black GS method for the Poisson problem
        sgrid::parallel_for(
            "LaplaceOp", g->md_range(), SGRID_LAMBDA(int i, int j) {
              if ((i + j) % 2 != color)
                return;

              Real scale_diag = -(2 / (hx * hx) + 2 / (hy * hy));
              Real acc =
                  (solution_dev.ref(i - 1, j) + solution_dev.ref(i + 1, j)) /
                      (hx * hx) +
                  (solution_dev.ref(i, j - 1) + solution_dev.ref(i, j + 1)) /
                      (hy * hy);

              solution_dev.ref(i, j) = (rhs_dev.ref(i, j) - acc) / scale_diag;
            });

        sgrid::fence();
      }

      solution.exchange_halos();

      if (iter % check_residual_each == 0 || (iter == (max_iter - 1))) {
        Real lg_norm = 0;

        // Matrix-free residual
        sgrid::parallel_reduce(
            "SquaredNormOfGradient", g->md_range(),
            SGRID_LAMBDA(int i, int j, Real &acc) {
              Real scale_diag = -(2 / (hx * hx) + 2 / (hy * hy));

              Real diag = scale_diag * solution_dev.ref(i, j);

              Real off_diag_x =
                  (solution_dev.ref(i - 1, j) + solution_dev.ref(i + 1, j)) /
                  (hx * hx);

              Real off_diag_y =
                  (solution_dev.ref(i, j - 1) + solution_dev.ref(i, j + 1)) /
                  (hy * hy);

              Real res = (rhs_dev.ref(i, j) - (diag + off_diag_x + off_diag_y));

              acc += res * res;
            },
            lg_norm);

        sgrid::fence();

        Real g_norm = 0;
        MPI_Allreduce(&lg_norm, &g_norm, 1, sgrid::MPIType<Real>(), MPI_SUM,
                      g->raw_comm());

        g_norm = std::sqrt(g_norm);

        if (rank == 0) {
          printf("Iteration %d, ||g|| = %g\n", iter, g_norm);
        }

        if (g_norm < atol) {
          break;
        }
      }
    }

    ////////////////////////////////////////////////////////////
    // Output
    ////////////////////////////////////////////////////////////

    Real end = MPI_Wtime();
    Real user_time = end - start;

    if (rank == 0) {
      printf("Device: \"%s\"\n", typeid(sgrid::DeviceExecutionSpace).name());
      printf("Time: %g (seconds)\n", user_time);
    }

    solution.synch_device_to_host();
    solution.write("data.raw");
  }

  sgrid::finalize();
  return MPI_Finalize();
}
