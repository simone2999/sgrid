#ifndef SGRID_SERIAL_PERIODIC_HALO_HPP
#define SGRID_SERIAL_PERIODIC_HALO_HPP

#include "sgrid_Halo.hpp"

namespace sgrid {

template <class Field, int Dim = Field::Grid::Dim>
class SerialPeriodicSideHalo final : public Halo {
public:
  SerialPeriodicSideHalo(Field &field, int) {
    // IMPLEMENT ME
    // MPI_Abort(field.grid()->raw_comm(), -1);
    if(field.grid()->comm_rank() == 0)
      printf("SerialPeriodicSideHalo unsupporterd for dim = %d\n", Dim);
  }

  void exchange() override {
    // IMPLEMENT ME
  }
};

template <class Field>
class SerialPeriodicSideHalo<Field, 3> final : public Halo {
public:
  void exchange() override {
    auto grid = field_.grid();

    if (grid->comm_dim(dim_) != 1) {
      MPI_Abort(grid->raw_comm(), -1);
    }

    auto slice = grid->md_range_slice_with_ghosts(dim_);
    auto x_dev = field_.view_device();
    int block_size = field_.block_size();

    /////////////////////////////////////////

    int offset[2] = {0, 0};

    switch (dim_) {
    case 0: {
      offset[0] = 1;
      offset[1] = 2;
      break;
    }
    case 1: {
      offset[0] = 0;
      offset[1] = 2;
      break;
    }
    case 2: {
      offset[0] = 0;
      offset[1] = 1;
      break;
    }
    default:
      break;
    }

    /////////////////////////////////////////

    auto g_dev = grid->view_device();

    /////////////////////////////////////////

    sgrid::parallel_for(
        "SliceLoop", slice, SGRID_LAMBDA(int i, int j) {
          int idx_from[3] = {0, 0, 0};
          int idx_to[3] = {0, 0, 0};

          idx_from[dim_] = 1;
          idx_to[dim_] = g_dev.dim_with_margin[dim_] - 1;

          idx_from[offset[0]] = i;
          idx_from[offset[1]] = j;

          idx_to[offset[0]] = i;
          idx_to[offset[1]] = j;

          auto *b_from = x_dev.p_block(idx_from);
          auto *b_to = x_dev.p_block(idx_to);

          for (int b = 0; b < block_size; ++b) {
            b_to[b] = b_from[b];
          }
        });

    sgrid::parallel_for(
        "SliceLoop", slice, SGRID_LAMBDA(int i, int j) {
          int idx_from[3] = {0, 0, 0};
          int idx_to[3] = {0, 0, 0};

          idx_from[dim_] = g_dev.dim_with_margin[dim_] - 2;
          idx_to[dim_] = 0;

          idx_from[offset[0]] = i;
          idx_from[offset[1]] = j;

          idx_to[offset[0]] = i;
          idx_to[offset[1]] = j;

          auto *b_from = x_dev.p_block(idx_from);
          auto *b_to = x_dev.p_block(idx_to);

          for (int b = 0; b < block_size; ++b) {
            b_to[b] = b_from[b];
          }
        });
  }

  SerialPeriodicSideHalo(Field &field, int dim) : field_(field), dim_(dim) {
    // std::cout << "SerialPeriodicSideHalo: " << dim << std::endl;
  }

private:
  Field &field_;
  int dim_;
};

} // namespace sgrid

#endif // SGRID_SERIAL_PERIODIC_HALO_HPP
