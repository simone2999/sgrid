#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_IO.hpp"
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
        bool verbose = false;

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
            printf("Comm grid (%d, %d, %d)\n", g->comm_dim(0), g->comm_dim(1), g->comm_dim(2));

            printf("nx=%d ny=%d nz=%d\n", nx, ny, nz);
        }

        auto g_dev = g->view_device();

        Field_t x("x", g, block_size, sgrid::BOX_STENCIL);
        // Field_t x("x", g, block_size, sgrid::STAR_STENCIL);

        x.allocate_on_device();

        auto x_dev = x.view_device();

        int rank = g->comm_rank();

        sgrid::parallel_for(
            "Index", g->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                ptrdiff_t x = g_dev.global_coord(0, i);  // 0=X
                ptrdiff_t y = g_dev.global_coord(1, j);  // 1=Y
                ptrdiff_t z = g_dev.global_coord(2, k);  // 2=Z

                auto b = x_dev.block(i, j, k);

                b[0] = x;
                b[1] = y;
                b[2] = z;
            });

        x.exchange_halos();

        long bugs = 0;

        for (int r = 0; r < g->comm_size(); ++r) {
            if (rank == r) {
                int px = g->comm_coord(0), py = g->comm_coord(1), pz = g->comm_coord(2);

                if (verbose) {
                    printf("--------------------------------\n");
                    printf("--------------------------------\n");
                    printf("rank=%d, comm_coord=[%d, %d, %d], dim=[%d/%d, %d/%d, %d/%d]\n",
                           rank,
                           // comm coords
                           px,
                           py,
                           pz,
                           g_dev.dim[0],
                           nx,
                           g_dev.dim[1],
                           ny,
                           g_dev.dim[2],
                           nz);
                    printf("--------------------------------\n");
                }

                int bug = 0;
                sgrid::parallel_reduce(
                    "Index",
                    g->md_range_with_ghosts(),
                    // "Index", g->md_range(),
                    SGRID_LAMBDA(int i, int j, int k, int &acc) {
                        const auto b = x_dev.block(i, j, k);

                        const ptrdiff_t xg = g_dev.global_coord(0, i);
                        const ptrdiff_t yg = g_dev.global_coord(1, j);
                        const ptrdiff_t zg = g_dev.global_coord(2, k);

                        int is_boundary_x = (xg == -1 || xg == nx);
                        int is_boundary_y = (yg == -1 || yg == ny);
                        int is_boundary_z = (zg == -1 || zg == nz);
                        int is_boundary = is_boundary_x + is_boundary_y + is_boundary_z;

                        bool fix_periodic = true;

                        ptrdiff_t x = xg;
                        ptrdiff_t y = yg;
                        ptrdiff_t z = zg;

                        if (fix_periodic) {
                            x = (x == -1) ? (x + nx) : x;
                            y = (y == -1) ? (y + ny) : y;
                            z = (z == -1) ? (z + nz) : z;

                            x = (x == nx) ? 0 : x;
                            y = (y == ny) ? 0 : y;
                            z = (z == nz) ? 0 : z;
                        }

                        assert(x >= 0);
                        assert(x < nx);

                        bool correct_value = x == b[0] && y == b[1] && z == b[2];

                        if (!correct_value) {
                            acc += 1;

                            if (!verbose) return;

                            int is_ghost_x = i == 0 || i == g_dev.dim[0] + 1;
                            int is_ghost_y = j == 0 || j == g_dev.dim[1] + 1;
                            int is_ghost_z = k == 0 || k == g_dev.dim[2] + 1;

                            int is_ghost = is_ghost_x + is_ghost_y + is_ghost_z;

                            // if (is_boundary != 1)
                            //   return;

                            if (is_boundary == 3) {
                                printf("CORNER\t");
                            } else if (is_boundary == 2) {
                                printf("EDGE\t");
                            } else if (is_boundary == 1) {
                                printf("SIDE\t");
                            } else {
                                printf("\t");
                            }

                            if (is_ghost) {
                                printf("(");
                            }

                            if (is_ghost == 1) {
                                printf("GHOST_SIDE");
                            } else if (is_ghost == 2) {
                                printf("GHOST_EDGE");
                            } else if (is_ghost == 3) {
                                printf("GHOST_CORNER");
                            } else {
                                printf("\t\t");
                            }

                            if (is_ghost) {
                                printf(")");
                            }

                            printf("\t");

                            printf(
                                "l(%d, %d, %d) -> (%ld, %ld, %ld) -> g(%ld, %ld, %ld) == "
                                "b=(%ld,%ld,%ld)\n",
                                // k, Local coords
                                i,
                                j,
                                k,
                                // Non periodic global coords
                                xg,
                                yg,
                                zg,
                                // Global coords
                                x,
                                y,
                                z,
                                // Value
                                b[0],
                                b[1],
                                b[2]);
                        }
                    },
                    bug);

                bugs += bug;
            }

            MPI_Barrier(MPI_COMM_WORLD);
        }

        MPI_Allreduce(MPI_IN_PLACE, &bugs, 1, sgrid::MPIType<long>(), MPI_SUM, g->raw_comm());

        if (g->comm_rank() == 0) {
            printf("Num bugs %ld\n", bugs);
        }
        // printf("Halo nz %d/%ld\n", bug, x_dev.data().size());
        //        x.write("example_4/x.raw");
        sgrid::IO io(x, "example_4", "data.raw");
        io.write();
    }

    Kokkos::finalize();
    return MPI_Finalize();
}