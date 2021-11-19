#ifndef SGRID_GRID_HPP
#define SGRID_GRID_HPP

#include "sgrid_Base.hpp"
#include "sgrid_Utils.hpp"

#include <mpi.h>
#include <type_traits>
#include <vector>

// https://wgropp.cs.illinois.edu/courses/cs598-s16/lectures/lecture32.pdf

namespace sgrid {

template <typename Real_, int Dim_> class Grid {
public:
  using GlobalOrdinal = ptrdiff_t;
  using LocalOrdinal = int;
  using Real = Real_;
  static constexpr int Dim = Dim_;

  using ViewHost = Kokkos::View<Real *, HostMemorySpace>;
  using ViewDevice = Kokkos::View<Real *, DeviceMemorySpace>;
  using IntD = Kokkos::View<LocalOrdinal[Dim], HostMemorySpace>;

  using MDRangeDevice =
      Kokkos::MDRangePolicy<Kokkos::Rank<Dim>, DeviceExecutionSpace>;

  using MDRangeHost =
      Kokkos::MDRangePolicy<Kokkos::Rank<Dim>, HostExecutionSpace>;

  static constexpr int dim() { return Dim; }

  template <class MemorySpace, class ExecutionSpace> class LocalGrid {
  public:
    using GView = Kokkos::View<GlobalOrdinal[Dim], MemorySpace>;
    using LView = Kokkos::View<LocalOrdinal[Dim], MemorySpace>;
    using MDRange = Kokkos::MDRangePolicy<Kokkos::Rank<Dim>, ExecutionSpace>;
    using Range = Kokkos::RangePolicy<ExecutionSpace>;

    // template <typename ToMemSpace, typename ToExecutionSpace,
    //           typename FromMemSpace, typename FromExecutionSpace>
    // friend void
    // deep_copy(LocalGrid<ToMemSpace, ToExecutionSpace> &to,
    //           const LocalGrid<FromMemSpace, FromExecutionSpace> &from) {
    //   Kokkos::deep_copy(to.global_dim, from.global_dim);
    //   Kokkos::deep_copy(to.start, from.start);
    //   Kokkos::deep_copy(to.margin, from.margin);
    //   Kokkos::deep_copy(to.dim, from.dim);
    //   Kokkos::deep_copy(to.dim_with_margin, from.dim_with_margin);
    // }

    template <typename FromMemSpace, typename FromExecutionSpace>
    void deep_copy(const LocalGrid<FromMemSpace, FromExecutionSpace> &from) {
      Kokkos::deep_copy(global_dim, from.global_dim);
      Kokkos::deep_copy(start, from.start);
      Kokkos::deep_copy(margin, from.margin);
      Kokkos::deep_copy(dim, from.dim);
      Kokkos::deep_copy(dim_with_margin, from.dim_with_margin);
    }

    MDRange md_range_with_ghosts() {
      typename MDRange::point_type start, end;

      for (int d = 0; d < Dim; ++d) {
        start[d] = 0;
        end[d] = dim[d] + 2 * margin[d];
      }

      return MDRange(start, end);
    }

    MDRange md_range() {
      typename MDRange::point_type start, end;

      for (int d = 0; d < Dim; ++d) {
        start[d] = margin[d];
        end[d] = dim[d] + margin[d];
      }

      return MDRange(start, end);
    }

    void init() {
      global_dim = GView("global_dim");
      start = GView("start");

      margin = LView("margin");
      dim = LView("dim");

      Kokkos::deep_copy(margin, 1);

      dim_with_margin = LView("dim_with_margin");
    }

    KOKKOS_INLINE_FUNCTION LocalOrdinal data_size() const {
      LocalOrdinal ret = 1;

      for (int d = 0; d < Dim; ++d) {
        ret *= dim_with_margin[d];
      }

      return ret;
    }

    // Comodity, but prioritize p_node_idx for more generic codes
    template <typename... Args>
    KOKKOS_INLINE_FUNCTION int node_idx(Args... args) const {
      static_assert(sizeof...(Args) == Dim,
                    "Number of arguments must be the same as Dim of grid!");
      return tensor_idx(dim_with_margin.data(), args...);
    }

    KOKKOS_INLINE_FUNCTION LocalOrdinal
    p_node_idx(const LocalOrdinal *idx) const {
      int stride = 1;
      int ret = 0;
      for (int d = Dim - 1; d >= 0; d--) {
        ret += idx[d] * stride;
        stride *= dim_with_margin[d];
      }

      return ret;
    }

    // Global indexing
    GView global_dim;
    GView start;

    // Local indexing
    LView margin;
    LView dim;
    LView dim_with_margin;
  };

  using LocalGridHost = LocalGrid<HostMemorySpace, HostExecutionSpace>;
  using LocalGridDevice = LocalGrid<DeviceMemorySpace, DeviceExecutionSpace>;

  MDRangeHost md_range_with_ghosts() {
    return grid_host_.md_range_with_ghosts();
  }

  MDRangeHost md_range() { return grid_host_.md_range(); }

  void init(MPI_Comm standard_comm, const std::vector<GlobalOrdinal> &dim,
            std::vector<int> periods = {}, std::vector<int> proc_dims = {}

  ) {
    if (periods.empty()) {
      periods.resize(Dim, 0);
    }

    if (proc_dims.empty()) {
      proc_dims.resize(Dim, 0);
    }

    grid_host_.init();
    grid_device_.init();

    coords_ = IntD("coords");
    proc_dims_ = IntD("proc_dims");
    periods_ = IntD("periods");

    for (int d = 0; d < Dim; ++d) {
      grid_host_.global_dim[d] = dim[d];
    }

    int size;
    MPI_Comm_size(standard_comm, &size);

    CATCH_MPI_ERROR(MPI_Dims_create(size, Dim, proc_dims.data()));

    CATCH_MPI_ERROR(MPI_Cart_create(standard_comm, Dim, proc_dims.data(),
                                    periods.data(), 1, &comm_));

    CATCH_MPI_ERROR(MPI_Cart_get(comm_, Dim, proc_dims.data(), periods.data(),
                                 coords_.data()));

    int cart_rank;
    MPI_Comm_rank(comm_, &cart_rank);

    for (int d = 0; d < Dim; ++d) {
      LocalOrdinal temp = grid_host_.global_dim[d] / proc_dims[d];
      LocalOrdinal modulo = grid_host_.global_dim[d] % proc_dims[d];
      grid_host_.dim[d] = temp + (coords_[d] < modulo);
      grid_host_.start[d] =
          temp * coords_[d] + std::min(modulo, LocalOrdinal(coords_[d]));

      grid_host_.dim_with_margin[d] =
          grid_host_.dim[d] + 2 * grid_host_.margin[d];
    }

    grid_device_.deep_copy(grid_host_);

    for (int d = 0; d < Dim; ++d) {
      proc_dims_[d] = proc_dims[d];
      periods_[d] = periods[d];
    }
  }

  template <typename... Args> int neigh(Args... args) {
    static_assert(sizeof...(Args) == Dim,
                  "Number of arguments must be the same as Dim of grid!");

    int coords[Dim];
    unpack_to_array(coords, 0, args...);

    return p_neigh(coords);
  }

  int p_neigh(int *coords) const {

    for (int d = 0; d < Dim; ++d) {
      coords[d] += coords_[d];

      if ((coords[d] < 0 || coords[d] >= proc_dims_[d]) && periods_[d] == 0) {
        return MPI_PROC_NULL;
      }
    }

    int r = 0;
    CATCH_MPI_ERROR(MPI_Cart_rank(comm_, coords, &r));
    return r;
  }

  int comm_rank() const {
    int rank;
    MPI_Comm_rank(comm_, &rank);
    return rank;
  }

  int comm_size() const {
    int size;
    MPI_Comm_size(comm_, &size);
    return size;
  }

  int comm_coord(int d) const { return coords_[d]; }
  int comm_dim(int d) const { return proc_dims_[d]; }

  int shift(int direction, int disp) const {
    int rank_source = comm_rank();
    int rank_dest = MPI_PROC_NULL;
    CATCH_MPI_ERROR(
        MPI_Cart_shift(comm_, direction, disp, &rank_source, &rank_dest));

    return rank_dest;
  }

  bool can_shift(int direction, int disp) const {
    return (shift(direction, disp) != MPI_PROC_NULL);
  }

  void describe() {

    int rank, size;

    MPI_Comm_rank(comm_, &rank);
    MPI_Comm_size(comm_, &size);

    for (int r = 0; r < size; ++r) {
      if (r == rank) {
        printf("=============================\n");
        printf("[%d]\t", rank);
        printf("\ncoords=(%d, %d, %d)\n", coords_[0], coords_[1], coords_[2]);

        if (r == 0) {
          printf("\nproc_dims\n");
          for (int d = 0; d < Dim; ++d) {
            printf("%d\t", proc_dims_[d]);
          }
          printf("\n");
        } else {
          printf("\n");
        }

        printf("\n(start,dim)\n");
        for (int d = 0; d < Dim; ++d) {
          printf("(%ld, %d)\t", grid_host_.start[d], grid_host_.dim[d]);
        }

        // int left = neigh(-1, 0);
        // int right = neigh(1, 0);
        // int diag = neigh(1, 1);

        // printf("\nleft=%d, right=%d, diag=%d\n", left, right, diag);

        printf("\n");
        printf("=============================\n");
      }
      MPI_Barrier(comm_);
    }
  }

  LocalGridHost &view_host() { return grid_host_; }
  LocalGridDevice &view_device() { return grid_device_; }

  inline MPI_Comm raw_comm() { return comm_; }

private:
  MPI_Comm comm_;
  IntD coords_;
  IntD proc_dims_;
  IntD periods_;
  LocalGridHost grid_host_;
  LocalGridDevice grid_device_;
};

} // namespace sgrid

#endif // SGRID_GRID_HPP