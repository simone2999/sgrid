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

int mandel_func(double cx, double cy) {
    int maxiter{500};
    int outofbounds{3};
    std::complex<double> c{cx, cy};
    std::complex<double> z = c;
    int i = 0;
    while (i < maxiter) {
        double n = std::norm(z);
        if (n > outofbounds) {
            break;
        }

        z = z * z + c;
        i++;
    }

    return i;
}

int simple_func(double x, double y, double z) { return x * x + y * y + z * z; }

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    sgrid::initialize(argc, argv);

    {
        // Define grid size
        int n_x = 10;
        int n_y = 10;
        int n_z = 1;
        int block_size = 3;

        if (argc >= 3) {
            n_x = atoi(argv[1]);
            n_y = atoi(argv[2]);
            n_z = atoi(argv[3]);
        }

        // Create grid
        auto grid = std::make_shared<Grid_t>();
        grid->init(MPI_COMM_WORLD, {n_x, n_y, n_z}, {1, 1, 0});

        // Create field from grid.
        LongIntField_t field("MandelBrotField", grid, block_size, sgrid::BOX_STENCIL);
        field.allocate_on_device();

        auto x_dev = field.view_device();
        auto g_dev = grid->view_device();

        // int rank = grid->comm_rank();

        int mod_x = n_x % grid->comm_dim(0);
        int offset_x = 0;

        for (int i_proc = 0; i_proc < grid->comm_coord(0); i_proc++) {
            offset_x += n_x / grid->comm_dim(0) + (i_proc < mod_x);
        }
        int mod_y = n_y % grid->comm_dim(1);
        int offset_y = 0;

        for (int i_proc = 0; i_proc < grid->comm_coord(1); i_proc++) {
            offset_y += n_y / grid->comm_dim(1) + (i_proc < mod_y);
        }
        int mod_z = n_z % grid->comm_dim(2);
        int offset_z = 0;

        for (int i_proc = 0; i_proc < grid->comm_coord(2); i_proc++) {
            offset_z += n_z / grid->comm_dim(2) + (i_proc < mod_z);
        }

        sgrid::parallel_for(
            "TEST", grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                auto b = x_dev.block(i, j, k);
                if (grid->comm_rank() == 1) {
                    std::cout << i << "," << j << "," << k << "," << std::endl;
                }
                int i_global = offset_x + i;
                int j_global = offset_y + j;
                int k_global = offset_z + k;

                b[0] = simple_func(i_global, j_global, k_global);
                b[1] = mandel_func(i_global, j_global);
            });

        sgrid::IO io(field, "example_16");
        io.write();
    }

    sgrid::finalize();
    return MPI_Finalize();
}