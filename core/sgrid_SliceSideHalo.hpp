#ifndef SGRID_SLICE_SIDE_HALO_HPP
#define SGRID_SLICE_SIDE_HALO_HPP

namespace sgrid {

// Exchange edges (3D) or corners of (2d) of a slice
template <class Field> class SliceSideHalo {
public:
  using Grid = typename Field::Grid;
  using ValueType = typename Field::ValueType;
  using LocalOrdinal = typename Field::LocalOrdinal;
  using GlobalOrdinal = typename Field::GlobalOrdinal;
  using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
  using HostMirror = typename ViewDevice::HostMirror;
  static constexpr int Dim = Grid::Dim;

  static_assert(Dim == 3, "Only supports 3D");

  /// 0=y(z)-plane, 1=x(z)-plane, (2=xy-plane)
  explicit SliceSideHalo(Field &field, int plane)
      : field_(field), plane_(plane) {
    init();
  }

  ~SliceSideHalo() { destroy(); }

  void exchange(int slice_number) {
    //
    auto grid = field_.grid();
    auto grid_host = grid->view_host();
    auto field_host = field_.view_host();

    int recv_idx[Dim];
    int send_idx[Dim];
    int dirs[2];
    int neigh_rank[2];
    int send_offsets[2];
    int recv_offsets[2];

    for (int dim = 0; dim < Dim; ++dim) {
      // Skip dim of the plane
      if (dim == plane_)
        continue;

      // Reset array starts
      for (int d = 0; d < Dim; ++d) {
        recv_idx[d] = grid_host.margin[d];
        send_idx[d] = grid_host.margin[d];
      }

      // Set plane coordinate to slice number
      recv_idx[plane_] = slice_number;
      send_idx[plane_] = slice_number;

      int proc_coord = grid->comm_coord(dim);
      bool is_even = proc_coord % 2 == 0;

      // Interlace communication
      dirs[0] = is_even ? -1 : 1;
      dirs[1] = is_even ? 1 : -1;

      // Find neighbors
      neigh_rank[0] = grid->shift(dim, dirs[0]);
      neigh_rank[1] = grid->shift(dim, dirs[1]);

      // Offsets for array
      send_offsets[is_even] = grid_host.margin[dim];
      send_offsets[!is_even] = grid_host.dim[dim]; // includes left margin

      recv_offsets[is_even] = 0;
      recv_offsets[!is_even] = grid_host.dim_with_margin[dim] - 1;

      int tag = 0;
      // Send/Recv

      for (int k = 0; k < 2; ++k) {
        recv_idx[dim] = recv_offsets[k];
        send_idx[dim] = send_offsets[k];

        auto recv_ptr = field_host.p_block(recv_idx);
        auto send_ptr = field_host.p_block(send_idx);

        CATCH_MPI_ERROR(MPI_Sendrecv(send_ptr, 1, send_type_[dim],
                                     neigh_rank[k], tag, recv_ptr, 1,
                                     recv_type_[dim], neigh_rank[k], tag,
                                     grid->raw_comm(), MPI_STATUS_IGNORE));
      }
    }

    // void wait_all() {}
  }

private:
  Field &field_;
  int plane_;

  // One type per side of the slice
  MPI_Datatype recv_type_[Dim];
  MPI_Datatype send_type_[Dim];

  void destroy() {
    for (auto &t : recv_type_) {
      if (t != MPI_DATATYPE_NULL) {
        MPI_Type_free(&t);
      }
      t = MPI_DATATYPE_NULL;
    }

    for (auto &t : send_type_) {
      if (t != MPI_DATATYPE_NULL) {
        MPI_Type_free(&t);
      }
      t = MPI_DATATYPE_NULL;
    }
  }

  void init() {
    // TODO check if dimension is serial and periodic

    for (auto &t : recv_type_) {
      t = MPI_DATATYPE_NULL;
    }

    for (auto &t : send_type_) {
      t = MPI_DATATYPE_NULL;
    }

    //
    auto grid = field_.grid();
    auto g_host = grid->view_host();

    auto field_host = field_.view_host();

    MPI_Datatype real_type = MPIType<ValueType>();

    for (int dim = 0; dim < Dim; ++dim) {
      // Skip dim of the plane
      if (dim == plane_)
        continue;

      int stride = 1;

      for (int d = Grid::Dim - 1; d > dim; --d) {
        stride *= g_host.dim_with_margin[d];
      }

      int count = g_host.dim[dim];
      int block_size = field_.block_size();

      if (stride == 1) {
        CATCH_MPI_ERROR(MPI_Type_contiguous(count * block_size, real_type,
                                            &recv_type_[dim]));

        CATCH_MPI_ERROR(MPI_Type_contiguous(count * block_size, real_type,
                                            &send_type_[dim]));
      } else {
        CATCH_MPI_ERROR(MPI_Type_vector(count, block_size, stride * block_size,
                                        real_type, &recv_type_[dim]));

        CATCH_MPI_ERROR(MPI_Type_vector(count, block_size, stride * block_size,
                                        real_type, &send_type_[dim]));
      }
    }
  }
};
} // namespace sgrid

#endif // SGRID_SLICE_SIDE_HALO_HPP
