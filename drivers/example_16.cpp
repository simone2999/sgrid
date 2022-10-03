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
    int maxiter{400};
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

        // Decrease -> Zoom in
        double x_min = -1.5;
        double x_max = 1.5;
        double y_min = -1.5;
        double y_max = 1.5;
        // double z_min = 0;
        // double z_max = 0;

        double max_zoom = 1;
        double time_steps = 0.01;
        double d_x = (x_max - x_min) / (n_x - 1);
        double d_y = (y_max - y_min) / (n_y - 1);

        // double d_z = (z_max - z_min) / (n_z - 1);
        sgrid::IO io(field, "example_16");

        int counter = 0;

        for (double i = -1; i < max_zoom; i += time_steps) {
            d_x = ((x_max - i) - (x_min + i)) / (n_x - 1);
            d_y = ((y_max - i) - (y_min + i)) / (n_y - 1);
            sgrid::parallel_for(
                "TEST", grid->md_range(), SGRID_LAMBDA(int i, int j, int k) {
                    auto b = x_dev.block(i, j, k);
                    int i_global = offset_x + i;
                    int j_global = offset_y + j;
                    // int k_global = offset_z + k;

                    double x = i_global * d_x;
                    double y = j_global * d_y;
                    // double z = z_min + k_global * d_z;

                    // b[0] = simple_func(i_global, j_global, k_global);
                    b[1] = mandel_func(y, x);
                });

            io.write("x_t" + std::to_string(counter) + ".raw");
            counter++;
        }

        // sgrid::parallel_for(
        //     "Test2", max_zoom, SGRID_LAMBDA(double i) { std::cout << "Hello world" << i << std::endl; });
    }

    sgrid::finalize();
    return MPI_Finalize();
}