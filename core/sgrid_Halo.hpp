#ifndef SGRID_HALO_HPP
#define SGRID_HALO_HPP

namespace sgrid {

class Halo {
public:
  virtual ~Halo() = default;
  virtual void exchange() = 0;
};

// 1D: 0
// 2D: 4 points
// 3D: 8 points
template <class Field> class NodeHalo : public Halo {
public:
  using Grid = typename Field::Grid;
  using Real = typename Field::Real;
  using LocalOrdinal = typename Field::LocalOrdinal;
  using GlobalOrdinal = typename Field::GlobalOrdinal;
  using ViewDevice = Kokkos::View<Real *, DeviceMemorySpace>;
  using HostMirror = typename ViewDevice::HostMirror;

  static constexpr int MaxDim = 3;

  static_assert(Grid::Dim <= MaxDim, "4D not supported yet!");

  bool init() { return true; }

  void exchange() override {
    int disp[MaxDim][2];
    int proc_coord_disp[MaxDim];
    auto grid = field_.grid();
    auto g_host = grid->view_host();

    auto field_host = field_.view_host();

    int block_size = field_.block_size();
    MPI_Datatype real_type = MPIType<Real>();

    // Generate displacements
    for (int d = 0; d < Grid::Dim; ++d) {
      for (int dir = 0; dir < 2; ++dir) {
        disp[d][dir] = (grid->comm_coord(d) % 2 == dir) ? -1 : 1;
      }
    }

    for (int d = Grid::Dim; d < MaxDim; ++d) {
      disp[d][0] = 0;
      disp[d][1] = 0;
    }

    int recv_idx[MaxDim];
    int send_idx[MaxDim];
    int tensor_idx[MaxDim];
    int n[MaxDim];

    for (int d = 0; d < Grid::Dim; ++d) {
      n[d] = 2;
    }

    for (int d = Grid::Dim; d < MaxDim; ++d) {
      n[d] = 1;
    }

    for (int i = 0; i < n[0]; ++i) {
      for (int j = 0; j < n[1]; ++j) {
        for (int k = 0; k < n[2]; ++k) {

          proc_coord_disp[0] = disp[0][i];
          proc_coord_disp[1] = disp[1][j];
          proc_coord_disp[2] = disp[2][k];

          const int neigh_rank = grid->p_neigh(proc_coord_disp);

          assert(neigh_rank != grid->comm_rank());

          if (neigh_rank == MPI_PROC_NULL)
            continue;

          tensor_idx[0] = i;
          tensor_idx[1] = j;
          tensor_idx[2] = k;

          for (int d = 0; d < Grid::Dim; ++d) {

            send_idx[d] = (disp[d][tensor_idx[d]] > 0)
                              ? (g_host.dim[d] - 1 + g_host.margin[d])
                              : g_host.margin[d];

            recv_idx[d] = (disp[d][tensor_idx[d]] < 0)
                              ? (g_host.dim[d] - 1 + 2 * g_host.margin[d])
                              : 0;
          }

          auto send_ptr = field_host.p_block(send_idx);
          auto recv_ptr = field_host.p_block(recv_idx);

          int tag = 0;

          // for (int r = 0; r < grid->comm_size(); ++r) {

          //   if (r == grid->comm_rank()) {
          //     printf("[%d] -> (%d, %d, %d) : neigh=%d -> (%d, %d, %d), "
          //            "disp=(%d, %d, %d)\n",
          //            grid->comm_rank(), grid->comm_coord(0),
          //            grid->comm_coord(1), grid->comm_coord(2), neigh_rank,
          //            proc_coord_disp[0], proc_coord_disp[1],
          //            proc_coord_disp[2], disp[0][i], disp[1][j], disp[2][k]);
          //   }

          //   // MPI_Barrier(grid->raw_comm());
          // }

          CATCH_MPI_ERROR(MPI_Sendrecv(send_ptr, block_size, real_type,
                                       neigh_rank, tag, recv_ptr, block_size,
                                       real_type, neigh_rank, tag,
                                       grid->raw_comm(), MPI_STATUS_IGNORE));
        }
      }
    }
  }

  NodeHalo(Field &field) : field_(field) {}

private:
  Field &field_;
};

// 1D: 0
// 2D: 0
// 3D: 12 edges
template <class Field> class EdgeHalo : public Halo {
public:
  using Grid = typename Field::Grid;
  using Real = typename Field::Real;
  using LocalOrdinal = typename Field::LocalOrdinal;
  using GlobalOrdinal = typename Field::GlobalOrdinal;
  using ViewDevice = Kokkos::View<Real *, DeviceMemorySpace>;
  using HostMirror = typename ViewDevice::HostMirror;

  bool init(int dim) {
    destroy();

    type_ = MPI_DATATYPE_NULL;

    if (Grid::Dim < 3)
      return false;

    auto grid = field_.grid();

    for (int d = 0; d < Grid::Dim; ++d) {
      if (dim == d)
        continue;

      if (grid->comm_dim(d) == 1) {
        // No communication necessary on this edge
        return false;
      }
    }

    dim_ = dim;

    for (int d = 0, idx = 0; d < Grid::Dim; ++d) {
      if (dim == d)
        continue;

      dims_[idx++] = d;
    }

    MPI_Datatype real_type = MPIType<Real>();

    auto g_host = grid->view_host();

    int stride = 1;

    for (int d = Grid::Dim - 1; d > dim; --d) {
      stride *= g_host.dim_with_margin[d];
    }

    int count = g_host.dim[dim];
    int block_size = field_.block_size();

    // printf("dim=%d, count=%d stride=%d\n", dim, count, stride);

    CATCH_MPI_ERROR(
        MPI_Type_vector(count, block_size, stride, real_type, &type_));

    MPI_Type_commit(&type_);

    return true;
  }

  void exchange() override {
    auto grid = field_.grid();
    auto g_host = grid->view_host();
    auto field_host = field_.view_host();

    LocalOrdinal send_idx[Grid::Dim];
    LocalOrdinal recv_idx[Grid::Dim];
    int proc_coord_disp[Grid::Dim];

    send_idx[dim_] = 0;
    recv_idx[dim_] = 0;

    int disp[Grid::Dim][2];

    for (int k = 0; k < Grid::Dim - 1; ++k) {
      int d = dims_[k];

      for (int dir = 0; dir < 2; ++dir) {
        disp[d][dir] = (grid->comm_coord(d) % 2 == dir) ? -1 : 1;
      }
    }

    if (Grid::Dim == 3) {

      for (int disp_num_0 = 0; disp_num_0 < 2; ++disp_num_0) {
        for (int disp_num_1 = 0; disp_num_1 < 2; ++disp_num_1) {
          const int d0 = dims_[0];
          const int d1 = dims_[1];

          assert(disp[d0][disp_num_0] != 0);
          assert(disp[d1][disp_num_1] != 0);

          proc_coord_disp[d0] = disp[d0][disp_num_0];
          proc_coord_disp[d1] = disp[d1][disp_num_1];
          proc_coord_disp[dim_] = 0;

          const int neigh_rank = grid->p_neigh(proc_coord_disp);

          assert(neigh_rank != grid->comm_rank());

          if (neigh_rank == MPI_PROC_NULL)
            continue;

          //////////////////////////////////////////////////////////////////////

          send_idx[d0] = (disp[d0][disp_num_0] > 0)
                             ? (g_host.dim[d0] - 1 + g_host.margin[d0])
                             : g_host.margin[d0];

          recv_idx[d0] = (disp[d0][disp_num_0] < 0)
                             ? (g_host.dim[d0] - 1 + 2 * g_host.margin[d0])
                             : 0;

          //////////////////////////////////////////////////////////////////////

          send_idx[d1] = (disp[d1][disp_num_0] > 0)
                             ? (g_host.dim[d1] - 1 + g_host.margin[d1])
                             : g_host.margin[d1];

          recv_idx[d1] = (disp[d1][disp_num_0] < 0)
                             ? (g_host.dim[d1] - 1 + 2 * g_host.margin[d1])
                             : 0;

          //////////////////////////////////////////////////////////////////////

          auto send_ptr = field_host.p_block(send_idx);
          auto recv_ptr = field_host.p_block(recv_idx);

          int tag = 0;

          // for (int r = 0; r < grid->comm_size(); ++r) {

          //   if (r == grid->comm_rank()) {

          //     printf("[%d] -> (%d, %d, %d) : neigh=%d -> (%d, %d, %d), d0=%d,
          //     "
          //            "d1=%d, disp=(%d, %d)\n",
          //            grid->comm_rank(), grid->comm_coord(0),
          //            grid->comm_coord(1), grid->comm_coord(2), neigh_rank,
          //            proc_coord_disp[0], proc_coord_disp[1],
          //            proc_coord_disp[2], d0, d1, disp[d0][disp_num_0],
          //            disp[d1][disp_num_1]);
          //   }

          //   MPI_Barrier(grid->raw_comm());
          // }

          CATCH_MPI_ERROR(MPI_Sendrecv(send_ptr, 1, type_, neigh_rank, tag,
                                       recv_ptr, 1, type_, neigh_rank, tag,
                                       grid->raw_comm(), MPI_STATUS_IGNORE));
        }
      }
    } else {
      assert(false && "IMPLEMENT ME");
      MPI_Abort(grid->raw_comm(), -1);
    }
  }

  EdgeHalo(Field &field) : field_(field) {}
  ~EdgeHalo() { destroy(); }

private:
  Field &field_;

  int dim_;
  int dims_[Grid::Dim - 1];
  MPI_Datatype type_{MPI_DATATYPE_NULL};

  void destroy() {
    if (type_ != MPI_DATATYPE_NULL) {
      MPI_Type_free(&type_);
    }
  }
};

// 1D: 2 sides
// 2D: 4 sides
// 3D: 6 sides
template <class Field> class SideHalo : public Halo {
public:
  using Grid = typename Field::Grid;
  using Real = typename Field::Real;
  using LocalOrdinal = typename Field::LocalOrdinal;
  using GlobalOrdinal = typename Field::GlobalOrdinal;
  using ViewDevice = Kokkos::View<Real *, DeviceMemorySpace>;
  using HostMirror = typename ViewDevice::HostMirror;

  SideHalo(Field &field) : field_(field) {}
  ~SideHalo() { destroy(); }

  /////////////////////////////////////////////////////////
  // Codimension 1
  bool init(int dim) {
    dim_ = dim;

    MPI_Datatype real_type = MPIType<Real>();

    auto grid = field_.grid();
    auto g_host = grid->view_host();

    for (int k = 0; k < 2; ++k) {
      recv_layer_type[k] = MPI_DATATYPE_NULL;
      send_layer_type[k] = MPI_DATATYPE_NULL;
    }

    if (grid->comm_dim(dim) == 1) {
      // No decomposition in this dimension
      return false;
    }

    int array_of_sizes[Grid::Dim + 1];
    int array_of_subsizes[Grid::Dim + 1];
    int array_of_starts[Grid::Dim + 1];

    array_of_sizes[Grid::Dim] = field_.block_size();
    array_of_subsizes[Grid::Dim] = field_.block_size();
    array_of_starts[Grid::Dim] = 0;

    for (int d = 0; d < Grid::Dim; ++d) {
      array_of_sizes[d] = g_host.dim[d] + 2 * g_host.margin[d];
    }

    LocalOrdinal recv_sides[2] = {0, 0};
    LocalOrdinal send_sides[2] = {0, 0};

    int neigh_rank[2];
    neigh_rank[0] = grid->shift(dim, -1);
    if (neigh_rank[0] != MPI_PROC_NULL) {
      recv_sides[0] = 0;
      send_sides[0] = 1;
    }

    neigh_rank[1] = grid->shift(dim, 1);
    if (neigh_rank[1] != MPI_PROC_NULL) {
      recv_sides[1] = g_host.dim[dim] + g_host.margin[dim];
      send_sides[1] = g_host.dim[dim] - 1 + g_host.margin[dim];
    }

    int n_sides = 2;
    for (int s = 0; s < n_sides; ++s) {
      if (neigh_rank[s] == MPI_PROC_NULL)
        continue;

      for (int d2 = 0; d2 < Grid::Dim; ++d2) {
        if (dim == d2)
          continue;

        array_of_subsizes[d2] = g_host.dim[d2];
        array_of_starts[d2] = g_host.margin[d2];
      }

      array_of_subsizes[dim] = 1;
      array_of_starts[dim] = recv_sides[s];

      int tensor_dir = Grid::Dim;

      if (field_.block_size() > 1) {
        tensor_dir++;
      }

      CATCH_MPI_ERROR(MPI_Type_create_subarray(
          tensor_dir, array_of_sizes, array_of_subsizes, array_of_starts,
          MPI_ORDER_C, real_type, &recv_layer_type[s]));

      array_of_subsizes[dim] = 1;
      array_of_starts[dim] = send_sides[s];

      CATCH_MPI_ERROR(MPI_Type_create_subarray(
          tensor_dir, array_of_sizes, array_of_subsizes, array_of_starts,
          MPI_ORDER_C, real_type, &send_layer_type[s]));

      MPI_Type_commit(&recv_layer_type[s]);
      MPI_Type_commit(&send_layer_type[s]);
    }

    return true;
  }

  void exchange() override {
    auto grid = field_.grid();
    auto field_host = field_.view_host();

    int coord = grid->comm_coord(dim_);
    int left = coord % 2 == 0;
    int right = coord % 2 == 1;

    assert(left != right);

    int tag = 0;

    MPI_Datatype recv_type[2];
    MPI_Datatype send_type[2];

    // LocalOrdinal recv_sides[2] = {0, 0};
    // LocalOrdinal send_sides[2] = {0, 0};
    int neigh_rank[2] = {MPI_PROC_NULL, MPI_PROC_NULL};

    neigh_rank[left] = grid->shift(dim_, -1);
    if (neigh_rank[left] != MPI_PROC_NULL) {
      recv_type[left] = recv_layer_type[0];
      send_type[left] = send_layer_type[0];
    }

    neigh_rank[right] = grid->shift(dim_, 1);
    if (neigh_rank[right] != MPI_PROC_NULL) {
      recv_type[right] = recv_layer_type[1];
      send_type[right] = send_layer_type[1];
    }

    for (int s = 0; s < 2; ++s) {
      if (neigh_rank[s] == MPI_PROC_NULL)
        continue;

      CATCH_MPI_ERROR(MPI_Sendrecv(field_host.ptr(), 1, send_type[s],
                                   neigh_rank[s], tag, field_host.ptr(), 1,
                                   recv_type[s], neigh_rank[s], tag,
                                   grid->raw_comm(), MPI_STATUS_IGNORE));
    }
  }

private:
  Field &field_;

  int dim_{0};
  MPI_Datatype recv_layer_type[2];
  MPI_Datatype send_layer_type[2];

  void destroy() {
    for (int s = 0; s < 2; ++s) {
      if (recv_layer_type[s] != MPI_DATATYPE_NULL) {
        MPI_Type_free(&recv_layer_type[s]);
      }

      if (send_layer_type[s] != MPI_DATATYPE_NULL) {
        MPI_Type_free(&send_layer_type[s]);
      }
    }
  }
};
} // namespace sgrid

#endif // SGRID_HALO_HPP
