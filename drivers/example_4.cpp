#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_View.hpp"

#include <mpi.h>

using Grid_t = sgrid::Grid<double, 3>;
using Field_t = sgrid::Field<Grid_t, ptrdiff_t>;

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  Kokkos::initialize(argc, argv);

  {
    int nx = 2;
    int ny = 3;
    int nz = 4;
    int block_size = 3;

    if (argc >= 3) {
      nx = atoi(argv[1]);
      ny = atoi(argv[2]);
      nz = atoi(argv[3]);
    }

    // bool perodic_local = true;
    bool perodic_local = false;

    auto g = std::make_shared<Grid_t>();

    if (perodic_local) {
      int comm_size;
      MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
      g->init(MPI_COMM_WORLD, {nx, ny, nz}, {1, 1, 1}, {1, 1, comm_size});
    } else {
      // g->init(MPI_COMM_WORLD, {nx, ny, nz}, {1, 1, 0});
      g->init(MPI_COMM_WORLD, {nx, ny, nz}, {1, 1, 1});
      // g->init(MPI_COMM_WORLD, {nx, ny, nz}, {0, 0, 0});
    }

    if (g->comm_rank() == 0) {
      printf("Comm grid (%d, %d, %d)\n", g->comm_dim(0), g->comm_dim(1),
             g->comm_dim(2));
    }

    auto g_dev = g->view_device();

    Field_t x("x", g, block_size, sgrid::BOX_STENCIL);
    // Field_t x("x", g, block_size, sgrid::STAR_STENCIL);

    x.allocate_on_device();

    auto x_dev = x.view_device();

    int rank = g->comm_rank();

    sgrid::parallel_for(
        "Index", g->md_range(), SGRID_LAMBDA(int i, int j, int k) {
          ptrdiff_t x = g_dev.global_coord(0, i);
          ptrdiff_t y = g_dev.global_coord(1, j);
          ptrdiff_t z = g_dev.global_coord(2, k);

          auto b = x_dev.block(i, j, k);

          b[0] = x;
          b[1] = y;
          b[2] = z;
        });

    x.exchange_halos();

    bool z_is_periodic = g->is_periodic(2);

    for (int r = 0; r < g->comm_size(); ++r) {

      if (rank == r) {

        int px = g->comm_coord(0), py = g->comm_coord(1), pz = g->comm_coord(2);

        printf("--------------------------------\n");
        printf("[%d][%d, %d, %d]\n", rank,
               // comm coords
               px, py, pz);

        int bug = 0;
        sgrid::parallel_reduce(
            "Index", g->md_range_with_ghosts(),
            // "Index", g->md_range(),
            SGRID_LAMBDA(int i, int j, int k, int &acc) {
              const auto b = x_dev.block(i, j, k);

              ptrdiff_t x = g_dev.global_coord(0, i);
              ptrdiff_t y = g_dev.global_coord(1, j);
              ptrdiff_t z = g_dev.global_coord(2, k);

              bool is_outer_ghosts[3] = {(x == -1) || (x == nx),
                                         (y == -1) || (y == ny),
                                         (z == -1) || (z == nz)};

              bool iog = is_outer_ghosts[0] || is_outer_ghosts[1] ||
                         is_outer_ghosts[2];

              bool fix_periodic = true;
              if (fix_periodic) {
                x = (x == -1) ? (x + nx) : x;
                y = (y == -1) ? (y + ny) : y;

                if (z_is_periodic) {
                  z = (z == -1) ? (z + nz) : z;
                } else {
                  z = 0;
                }

                x = (x == nx) ? 0 : x;
                y = (y == ny) ? 0 : y;

                if (z_is_periodic) {
                  z = (z == nz) ? 0 : z;
                } else {
                  z = 0;
                }
              }

              assert(x >= 0);
              assert(x < nx);

              bool correct_value = x == b[0] && y == b[1] && z == b[2];

              if (!correct_value) {
                acc += 1;

                bool is_ghost_left = i == 0 || j == 0 || k == 0;
                bool is_ghost_right = i == g_dev.dim[0] + 1 ||
                                      j == g_dev.dim[1] + 1 ||
                                      k == g_dev.dim[2] + 1;

                bool is_ghost = is_ghost_left || is_ghost_right;

                int is_boundary = (x == -1 || y == -1) ||
                                  (x == nx || y == ny) || (z == -1 || z == nz);

                if (iog) {
                  printf("IOG:\t");
                } else if (is_boundary) {
                  printf("B: \t\t");
                } else if (is_ghost) {
                  printf("G: \t\t\t");
                }

                printf("l(%d, %d, %d) -> g(%ld, %ld, %ld) == "
                       "<%ld,%ld,%ld>\n",
                       // k, Local coords
                       i, j, k,
                       // Global coords
                       x, y, z,
                       // Value
                       b[0], b[1], b[2]);
              }
            },
            bug);
      }

      MPI_Barrier(MPI_COMM_WORLD);
    }
    // printf("Halo nz %d/%ld\n", bug, x_dev.data().size());
    x.write("x.raw");
  }

  Kokkos::finalize();
  return MPI_Finalize();
}