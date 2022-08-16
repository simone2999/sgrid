#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_IO.hpp"
#include "sgrid_View.hpp"

#include <mpi.h>

using Grid_t = sgrid::Grid<double, 2>;
using Field_t = sgrid::Field<Grid_t, double>;
const std::string file_name = "x.raw";
const std::string folder_name = "example_5";
const std::filesystem::path folder_path = folder_name;

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    Kokkos::initialize(argc, argv);

    {
        int nx = 2;
        int ny = 3;
        int block_size = 3;

        if (argc >= 3) {
            nx = atoi(argv[1]);
            ny = atoi(argv[2]);
        }

        // bool perodic_local = true;
        bool perodic_local = false;

        auto g = std::make_shared<Grid_t>();

        if (perodic_local) {
            int comm_size;
            MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
            g->init(MPI_COMM_WORLD, {nx, ny}, {1, 1}, {1, comm_size});
        } else {
            g->init(MPI_COMM_WORLD, {nx, ny}, {1, 1});
            // g->init(MPI_COMM_WORLD, {nx, ny}, {0, 0});
        }

        if (g->comm_rank() == 0) {
            printf("Comm grid (%d, %d)\n", g->comm_dim(0), g->comm_dim(1));
            printf("nx=%d ny=%d\n", nx, ny);
        }

        auto g_dev = g->view_device();

        Field_t x("x", g, block_size, sgrid::BOX_STENCIL);
        // Field_t x("x", g, block_size, sgrid::STAR_STENCIL);

        x.allocate_on_device();

        auto x_dev = x.view_device();

        int rank = g->comm_rank();

        sgrid::parallel_for(
            "Index", g->md_range_with_ghosts(), SGRID_LAMBDA(int i, int j) {
                auto b = x_dev.block(i, j);

                b[0] = -2;
                b[1] = -2;
                b[2] = -2;
            });

        sgrid::parallel_for(
            "Index", g->md_range(), SGRID_LAMBDA(int i, int j) {
                ptrdiff_t x = g_dev.global_coord(0, i);  // 0=X
                ptrdiff_t y = g_dev.global_coord(1, j);  // 1=Y

                auto b = x_dev.block(i, j);

                b[0] = x;
                b[1] = y;
                b[2] = rank;
            });

        x.exchange_halos();

        for (int r = 0; r < g->comm_size(); ++r) {
            if (rank == r) {
                int px = g->comm_coord(0), py = g->comm_coord(1);

                printf("--------------------------------\n");
                printf("--------------------------------\n");
                printf("rank=%d, comm_coord=[%d, %d], dim=[%d/%d, %d/%d]\n",
                       rank,
                       // comm coords
                       px,
                       py,
                       g_dev.dim[0],
                       nx,
                       g_dev.dim[1],
                       ny);
                printf("--------------------------------\n");

                int bug = 0;
                sgrid::parallel_reduce(
                    "Index",
                    g->md_range_with_ghosts(),
                    // "Index", g->md_range(),
                    SGRID_LAMBDA(int i, int j, int &acc) {
                        const auto b = x_dev.block(i, j);

                        const ptrdiff_t xg = g_dev.global_coord(0, i);
                        const ptrdiff_t yg = g_dev.global_coord(1, j);

                        int is_boundary_x = (xg == -1 || xg == nx);
                        int is_boundary_y = (yg == -1 || yg == ny);

                        int is_boundary = is_boundary_x + is_boundary_y;

                        bool fix_periodic = true;

                        ptrdiff_t x = xg;
                        ptrdiff_t y = yg;

                        if (fix_periodic) {
                            x = (x == -1) ? (x + nx) : x;
                            y = (y == -1) ? (y + ny) : y;

                            x = (x == nx) ? 0 : x;
                            y = (y == ny) ? 0 : y;
                        }

                        assert(x >= 0);
                        assert(x < nx);

                        bool correct_value = x == b[0] && y == b[1];

                        if (!correct_value) {
                            acc += 1;

                            int is_ghost_x = i == 0 || i == g_dev.dim[0] + 1;
                            int is_ghost_y = j == 0 || j == g_dev.dim[1] + 1;

                            int is_ghost = is_ghost_x + is_ghost_y;

                            // if (is_boundary != 1)
                            //   return;

                            if (is_boundary == 2) {
                                printf("CORNER\t");
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
                                "l(%d, %d) -> (%ld, %ld) -> g(%ld, %ld) == "
                                "b=(%ld,%ld)\n",
                                // k, Local coords
                                i,
                                j,
                                // Non periodic global coords
                                xg,
                                yg,
                                // Global coords
                                x,
                                y,
                                // Value
                                ptrdiff_t(b[0]),
                                ptrdiff_t(b[1]));
                        }
                    },
                    bug);
            }

            MPI_Barrier(MPI_COMM_WORLD);
        }
        // printf("Halo nz %d/%ld\n", bug, x_dev.data().size());
        x.write(folder_name + "/" + file_name);

        if (rank == 0) {
            sgrid::IO io(nx, ny, 0, block_size, folder_name);
            io.write();
        }

        sgrid::RawIODebug<Field_t> debug_out(x);
        debug_out.set_output_path("example_5/x_debug.raw");
        debug_out.write();
        MPI_Barrier(MPI_COMM_WORLD);
    }

    Kokkos::finalize();
    return MPI_Finalize();
}