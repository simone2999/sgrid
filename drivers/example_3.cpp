#include <filesystem>
#include <fstream>
#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_IO.hpp"
#include "sgrid_View.hpp"

#include <mpi.h>
#include <fstream>

using Real = double;

using Grid_t = sgrid::Grid<Real, 3>;
using LongIntField_t = sgrid::Field<Grid_t, long>;

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    sgrid::initialize(argc, argv);
    {
        int N_x = 4;
        int N_y = 4;
        int N_z = 10;
        int block_size = 3;

        if (argc >= 3) {
            N_x = atoi(argv[1]);
            N_y = atoi(argv[2]);
            N_z = atoi(argv[3]);
        }

        // if (argc >= 5) {
        //     block_size = atoi(argv[4]);
        // }

        auto g = std::make_shared<Grid_t>();
        g->init(MPI_COMM_WORLD, {N_x, N_y, N_z}, {1, 1, 0});

        LongIntField_t x("index", g, block_size, sgrid::BOX_STENCIL);
        x.allocate_on_device();

        int rank = g->comm_rank();

        // auto horizontal_slice = g->md_range_slice(2);

        auto x_dev = x.view_device();
        auto g_dev = g->view_device();

        sgrid::parallel_for(
            "TEST", g->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                auto b = x_dev.block(i, j, k);
                // Local index z coordinate
                b[0] = k;
                // process rank
                b[1] = rank;
                // Global index of z-coordinate
                b[2] = g_dev.start[2] + k - g_dev.margin[2];
            });

        //        x.write("x.raw");
        // For now, it creates n IO objects, don't know if this is correct
        // Writing of metadata checks if rank is 0 though.
        sgrid::IO io(x, "example_3");
        io.write();
    }

    sgrid::finalize();
    return MPI_Finalize();
}