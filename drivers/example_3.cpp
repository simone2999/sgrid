#include <fstream>
#include "sgrid_Base.hpp"
#include "sgrid_Field.hpp"
#include "sgrid_View.hpp"

#include "sgrid_DataExport.hpp"

#include <mpi.h>
#include <fstream>

using Real = double;

using Grid_t = sgrid::Grid<Real, 3>;
using LongIntField_t = sgrid::Field<Grid_t, long>;
std::string folder_name = "x.raw";

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    sgrid::initialize(argc, argv);

    {
        const int N_x = 4;
        const int N_y = 4;
        const int N_z = 10;
        const int block_size = 3;

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

        // Check if folder exists, not, then create then populate.
        x.write(folder_name);

        if (rank == 0) {
            data_export example3;
            example3.nx = N_x;
            example3.ny = N_y;
            example3.nz = N_z;
            example3.block_size = block_size;
            example3.endianess = "Little";
            sgrid::DataExport d(example3);
            d.create_header();
        }
        // if (rank == 0) {
        //     std::ifstream ifs("../xdmf_template.txt");
        //     std::string xdmf_string;
        //     xdmf_string.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        // }
        // struct with all variables(nx, ny, nz, block_size, endianess), then write .yml. Then python to convert
        // with xml parser.
    }

    sgrid::finalize();
    return MPI_Finalize();
}